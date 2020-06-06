#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#ifndef MAX_CONCURRENCY
#define MAX_CONCURRENCY 4
#endif // MAX_CONCURRENCY

#ifndef PIPELINING_SIZE
#define PIPELINING_SIZE 10
#endif // PIPELINING_SIZE

typedef struct { int copied, skipped, failed, found; } MigrationResult;
typedef struct { Cluster *src, *dest; int i, firstSlot, lastSlot, dryRun; MigrationResult *result; } WorkerArgs;
typedef struct { char buf[MAX_CMD_SIZE * PIPELINING_SIZE]; int i, cnt; } Pipeline;

#define ASSERT_MIGRATION_DATA(n, m) do {\
  if (n != m) {\
    fprintf(stderr, "RESTORE: keys and dump values are mismatched\n");\
    exit(1);\
  }\
} while (0)

static void countRestoreResult(const Reply *reply, MigrationResult *result) {
  int i;

  for (i = 0; i < reply->i; ++i) {
    if (strncmp(reply->lines[i], "OK", 2) == 0) {
      result->copied++;
    } else {
      result->failed++;
    }
  }
}

static void transferKeys(Conn *c, const Reply *keys, const Reply *values, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret;

  ASSERT_MIGRATION_DATA(keys->i, values->i);

  for (i = 0; i < keys->i; ++i) {
    if (values->types[i] == NIL) {
      result->skipped++;
      continue;
    }

    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2 + values->sizes[i],
        "RESTORE %s 0 \"%s\" REPLACE\r\n", keys->lines[i], values->lines[i]);
    pip.cnt++;
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    pip.buf[pip.i] = '\0';
    ret = pipeline(c, pip.buf, &reply, pip.cnt);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      countRestoreResult(&reply, result);
    }
    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

static void copyKeys(Conn *src, Conn *dest, const Reply *keys, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret;

  for (i = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2, "DUMP %s\r\n", keys->lines[i]);
    pip.cnt++;
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    pip.buf[pip.i] = '\0';
    ret = pipeline(src, pip.buf, &reply, pip.cnt);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      transferKeys(dest, keys, &reply, result);
    }
    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

static int migrateKeys(const Cluster *src, const Cluster *dest, int slot, int dryRun, MigrationResult *result) {
  char buf[MAX_CMD_SIZE];
  int ret;
  Reply reply;

  ret = countKeysInSlot(FIND_CONN(src, slot), slot);
  if (ret == MY_ERR_CODE) return ret;
  if (ret == 0) return MY_OK_CODE;
  result->found += ret;
  if (dryRun) return MY_OK_CODE;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", slot, ret);
  ret = command(FIND_CONN(src, slot), buf, &reply);
  if (ret == MY_ERR_CODE) {
    freeReply(&reply);
    return ret;
  }
  copyKeys(FIND_CONN(src, slot), FIND_CONN(dest, slot), &reply, result);
  freeReply(&reply);

  return MY_OK_CODE;
}

static void *workOnATask(void *args) {
  WorkerArgs *p;
  int i;

  p = (WorkerArgs *) args;
  printf("%02d: %lu <%lu>: %05d - %05d\n", p->i, (unsigned long) getpid(), (unsigned long) pthread_self(), p->firstSlot, p->lastSlot);
  p->result->found = p->result->copied = p->result->skipped = p->result->failed = 0;
  for (i = p->firstSlot; i <= p->lastSlot; ++i) migrateKeys(p->src, p->dest, i, p->dryRun, p->result);
  pthread_exit((void *) p->result);
}

static int migrateKeysPerSlot(const Cluster *src, const Cluster *dest, int dryRun) {
  int i, ret, chunk;
  void *tmp;
  pthread_t workers[MAX_CONCURRENCY];
  WorkerArgs args[MAX_CONCURRENCY];
  MigrationResult sum, results[MAX_CONCURRENCY];

  if (CLUSTER_SLOT_SIZE % MAX_CONCURRENCY > 0) {
    fprintf(stderr, "MAX_CONCURRENCY must be %d's divisor: %d given", CLUSTER_SLOT_SIZE, MAX_CONCURRENCY);
    return MY_ERR_CODE;
  }

  chunk = CLUSTER_SLOT_SIZE / MAX_CONCURRENCY;
  sum.found = sum.copied = sum.skipped = sum.failed = 0;

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    args[i].src = copyClusterState(src);
    if (args[i].src == NULL) return MY_ERR_CODE;

    args[i].dest = copyClusterState(dest);
    if (args[i].dest == NULL) return MY_ERR_CODE;

    args[i].i = i + 1;
    args[i].firstSlot = i * chunk;
    args[i].lastSlot = i * chunk + chunk - 1;
    args[i].dryRun = dryRun;
    args[i].result = &results[i];

    ret = pthread_create(&workers[i], NULL, workOnATask, &args[i]);
    if (ret != 0) {
      fprintf(stderr, "pthread_create(3): Could not create a worker");
      return MY_ERR_CODE;
    }
  }

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    ret = pthread_join(workers[i], &tmp);
    if (ret != 0) {
      fprintf(stderr, "pthread_join(3): Could not join with a thread");
      return MY_ERR_CODE;
    }

    sum.found += results[i].found;
    sum.copied += results[i].copied;
    sum.skipped += results[i].skipped;
    sum.failed += results[i].failed;
  }

  printf("%d keys were found\n", sum.found);
  printf("%d keys were copied\n", sum.copied);
  printf("%d keys were skipped\n", sum.skipped);
  printf("%d keys were failed\n", sum.failed);

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    if (freeClusterState(args[i].src) == MY_ERR_CODE) return MY_ERR_CODE;
    if (freeClusterState(args[i].dest) == MY_ERR_CODE) return MY_ERR_CODE;
    free(args[i].src);
    free(args[i].dest);
  }

  return MY_OK_CODE;
}

int main(int argc, char **argv) {
  int ret, dryRun;
  Cluster src, dest;
  struct sigaction sa;

  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Usage: bin/exe src-host:port dest-host:port [-C]\n");
    fprintf(stderr, "  -C   dry run\n");
    exit(1);
  }

  dryRun = argc == 4 ? 1 : 0;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    fprintf(stderr, "sigaction(2): SIGPIPE failed");
    exit(1);
  }

  ret = fetchClusterState(argv[1], &src);
  ASSERT(ret);

  ret = fetchClusterState(argv[2], &dest);
  ASSERT(ret);

  ret = migrateKeysPerSlot(&src, &dest, dryRun);
  ASSERT(ret);

  ret = freeClusterState(&src);
  ASSERT(ret);

  ret = freeClusterState(&dest);
  ASSERT(ret);

  exit(0);
}
