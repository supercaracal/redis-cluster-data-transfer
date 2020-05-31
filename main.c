#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#define MIGRATE_CMD_TIMEOUT 30000
#define MAX_CONCURRENCY 16

typedef struct { int copied, failed, found; } MigrationResult;
typedef struct { Cluster *src, *dest; int firstSlot, lastSlot, dryRun; MigrationResult *result; } WorkerArgs;

static int countKeysInSlot(const Conn *conn, int slot) {
  char buf[MAX_CMD_SIZE], *line;
  int ret;
  Reply reply;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER COUNTKEYSINSLOT %d", slot);
  ret = command(conn, buf, &reply);
  if (ret == MY_ERR_CODE) return ret;
  line = LAST_LINE2(reply);
  ret = line == NULL ? 0 : atoi(line);
  freeReply(&reply);

  return ret;
}

static int copyKey(const Conn *src, const Conn *dest, const char *key) {
  char buf[MAX_CMD_SIZE];
  int ret;
  Reply reply;

  snprintf(buf, MAX_CMD_SIZE, "MIGRATE %s %s %s 0 %d COPY REPLACE", dest->addr.host, dest->addr.port, key, MIGRATE_CMD_TIMEOUT);
  ret = command(src, buf, &reply);
  if (ret == MY_ERR_CODE) return ret;
  freeReply(&reply);

  return MY_OK_CODE;
}

static int migrateKeys(const Cluster *src, const Cluster *dest, int slot, int dryRun, MigrationResult *result) {
  char buf[MAX_CMD_SIZE];
  int i, ret;
  Reply reply;

  ret = countKeysInSlot(FIND_CONN(src, slot), slot);
  if (ret == MY_ERR_CODE) return ret;
  if (ret == 0) return MY_OK_CODE;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", slot, ret);
  ret = command(FIND_CONN(src, slot), buf, &reply);
  if (ret == MY_ERR_CODE) return ret;

  for (i = 0; i < reply.i; ++i) {
    if (dryRun) {
      result->found++;
      continue;
    }
    ret = copyKey(FIND_CONN(src, slot), FIND_CONN(dest, slot), reply.lines[i]);
    ret == MY_ERR_CODE ? result->failed++ : result->copied++;
  }

  freeReply(&reply);

  return MY_OK_CODE;
}

static void *workOnATask(void *args) {
  int i;
  WorkerArgs *p;

  p = (WorkerArgs *) args;
  for (i = p->firstSlot; i <= p->lastSlot; ++i) migrateKeys(p->src, p->dest, i, p->dryRun, p->result);
  pthread_exit((void *) p->result);
}

static int migrateKeysPerSlot(const Cluster *src, const Cluster *dest, int dryRun) {
  int i, ret, chunk;
  pid_t pid;
  void *tmp;
  pthread_t workers[MAX_CONCURRENCY];
  WorkerArgs args[MAX_CONCURRENCY];
  MigrationResult sum, results[MAX_CONCURRENCY];

  if (CLUSTER_SLOT_SIZE % MAX_CONCURRENCY > 0) {
    fprintf(stderr, "MAX_CONCURRENCY must be %d's divisor: %d given", CLUSTER_SLOT_SIZE, MAX_CONCURRENCY);
    return MY_ERR_CODE;
  }

  chunk = CLUSTER_SLOT_SIZE / MAX_CONCURRENCY;
  sum.found = sum.copied = sum.failed = 0;
  pid = getpid();

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    if (copyClusterState(src, args[i].src) == MY_ERR_CODE) return MY_ERR_CODE;
    if (copyClusterState(dest, args[i].dest) == MY_ERR_CODE) return MY_ERR_CODE;
    args[i].firstSlot = i * chunk;
    args[i].lastSlot = i * chunk + chunk - 1;
    args[i].dryRun = dryRun;
    args[i].result = &results[i];

    ret = pthread_create(&workers[i], NULL, workOnATask, &args[i]);
    if (ret != 0) {
      fprintf(stderr, "pthread_create(3): Could not create a worker");
      return MY_ERR_CODE;
    }

    printf("%lu <%lu>: %05d - %05d\n", (unsigned long) pid, (unsigned long) workers[i], args[i].firstSlot, args[i].lastSlot);
  }

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    ret = pthread_join(workers[i], &tmp);
    if (ret != 0) {
      fprintf(stderr, "pthread_join(3): Could not join with thread");
      return MY_ERR_CODE;
    }

    sum.found += results[i].found;
    sum.copied += results[i].copied;
    sum.failed += results[i].failed;
  }

  if (dryRun) {
    printf("%d keys were found\n", sum.found);
  } else {
    printf("%d keys were copied\n", sum.copied);
    printf("%d keys were failed\n", sum.failed);
  }

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

  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Usage: bin/exe src-host:port dest-host:port [-C]\n");
    fprintf(stderr, "  -C   dry run\n");
    exit(1);
  }

  dryRun = argc == 4 ? 1 : 0;

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
