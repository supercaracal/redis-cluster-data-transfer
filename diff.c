#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./command_raw.h"
#include "./cluster.h"

#ifndef MAX_CONCURRENCY
#define MAX_CONCURRENCY 4
#endif  // MAX_CONCURRENCY

#ifndef PIPELINING_SIZE
#define PIPELINING_SIZE 10
#endif  // PIPELINING_SIZE

#define RDB_VER_BYTES 10

typedef struct { int found, same, different, deficient, failed, skipped; } DiffResult;
typedef struct { Cluster *src, *dest; int i, firstSlot, lastSlot; DiffResult *result; } WorkerArgs;
typedef struct { char buf[MAX_CMD_SIZE * PIPELINING_SIZE]; int i, cnt; } Pipeline;

static inline int compareBytes(const char *a, int sizeA, const char *b, int sizeB) {
  int i;

  if (sizeA != sizeB) return 1;
  for (i = 0; i < sizeA - RDB_VER_BYTES; ++i) {
    if (a[i] != b[i]) return 1;
  }
  return 0;
}

static void compareValues(const Reply *a, const Reply *b, DiffResult *result) {
  int i, ret;

  for (i = 0; i < a->i; i += 2) {
    if (a->types[i] != RAW || b->types[i] != RAW || a->sizes[i] != b->sizes[i] ||
        compareBytes(a->lines[i], a->sizes[i], b->lines[i], b->sizes[i])) {
      fprintf(stderr, "Fishy key was given. Something was wrong.\n");
      exit(1);
    }

    if (a->types[i+1] == NIL) {
      result->skipped++;
      continue;
    }

    if (a->types[i+1] == RAW && b->types[i+1] == NIL) {
      result->deficient++;
      continue;
    }

    ret = compareBytes(a->lines[i+1], a->sizes[i+1], b->lines[i+1], b->sizes[i+1]);
    if (ret == 0) {
      result->same++;
    } else {
      result->different++;

      printf("[Key]-------------------------------------------------------------\n");
      PRINT_MIXED_BINARY(a->lines[i], a->sizes[i]);
      printf("\n");
      printf("[Source]----------------------------------------------------------\n");
      PRINT_BINARY(a->lines[i+1], a->sizes[i+1]);
      printf("\n");
      printf("[Destination]-----------------------------------------------------\n");
      PRINT_BINARY(b->lines[i+1], b->sizes[i+1]);
      printf("\n");
      exit(1);
    }
  }
}

static int fetchValues(Conn *conn, const Pipeline *pip, Reply *reply) {
  int ret;

  ret = commandWithRawData(conn, pip->buf, reply, pip->i);
  if (ret == MY_ERR_CODE) return ret;

  while (reply->i < pip->cnt * 2) {
    ret = readRemainedReplyLines(conn, reply);
    if (ret == MY_ERR_CODE) break;
  }
  if (ret == MY_ERR_CODE) return ret;
  if (reply->i != pip->cnt * 2) {
    PRINT_MIXED_BINARY(pip->buf, pip->i); printf("\n");
    printReplyLines(reply);
    fprintf(stderr, "DUMP: lack or too much reply");
    exit(1);
  }

  return MY_OK_CODE;
}

static void matchKeysAgainst(Conn *src, Conn *dest, const Reply *keys, DiffResult *result) {
  Pipeline pip;
  Reply valsA, valsB;
  int i, ret;

  for (i = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2, "ECHO %s\r\nDUMP %s\r\n", keys->lines[i], keys->lines[i]);
    pip.cnt++;

    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    ret = fetchValues(src, &pip, &valsA);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
      pip.cnt = pip.i = 0;
      freeReply(&valsA);
      continue;
    }

    ret = fetchValues(dest, &pip, &valsB);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
      pip.cnt = pip.i = 0;
      freeReply(&valsA);
      freeReply(&valsB);
      continue;
    }

    compareValues(&valsA, &valsB, result);
    pip.cnt = pip.i = 0;
    freeReply(&valsA);
    freeReply(&valsB);
  }
}

static int matchSlotAgainst(Conn *src, Conn *dest, int slot, DiffResult *result) {
  char buf[MAX_CMD_SIZE];
  int ret, cnt;
  Reply reply;

  ret = countKeysInSlot(src, slot);
  if (ret == MY_ERR_CODE) return ret;
  if (ret == 0) return MY_OK_CODE;
  result->found += ret;

  cnt = ret;
  snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", slot, ret);
  ret = command(src, buf, &reply);
  if (ret == MY_ERR_CODE) {
    result->failed += cnt;
    freeReply(&reply);
    return ret;
  }

  matchKeysAgainst(src, dest, &reply, result);
  freeReply(&reply);

  return MY_OK_CODE;
}

static void *workOnATask(void *args) {
  int i;
  WorkerArgs *p;

  p = (WorkerArgs *) args;
  p->result->found = p->result->same = p->result->different = p->result->deficient =
    p->result->failed = p->result->skipped = 0;

  printf("%02d: %lu <%lu>: %05d - %05d\n",
      p->i, (unsigned long) getpid(), (unsigned long) pthread_self(), p->firstSlot, p->lastSlot);

  for (i = p->firstSlot; i <= p->lastSlot; ++i) {
    matchSlotAgainst(FIND_CONN(p->src, i), FIND_CONN(p->dest, i), i, p->result);
  }

  pthread_exit((void *) p->result);
}

static int takeDiff(const Cluster *src, const Cluster *dest) {
  int i, ret, chunk;
  void *tmp;
  pthread_t workers[MAX_CONCURRENCY];
  WorkerArgs args[MAX_CONCURRENCY];
  DiffResult sum, results[MAX_CONCURRENCY];

  if (CLUSTER_SLOT_SIZE % MAX_CONCURRENCY > 0) {
    fprintf(stderr, "MAX_CONCURRENCY must be %d's divisor: %d given\n", CLUSTER_SLOT_SIZE, MAX_CONCURRENCY);
    return MY_ERR_CODE;
  }

  chunk = CLUSTER_SLOT_SIZE / MAX_CONCURRENCY;
  sum.found = sum.same = sum.different = sum.deficient = sum.failed = sum.skipped = 0;

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    args[i].src = copyClusterState(src);
    if (args[i].src == NULL) return MY_ERR_CODE;

    args[i].dest = copyClusterState(dest);
    if (args[i].dest == NULL) return MY_ERR_CODE;

    args[i].i = i + 1;
    args[i].firstSlot = i * chunk;
    args[i].lastSlot = i * chunk + chunk - 1;
    args[i].result = &results[i];

    ret = pthread_create(&workers[i], NULL, workOnATask, &args[i]);
    if (ret != 0) {
      fprintf(stderr, "pthread_create(3): Could not create a worker\n");
      return MY_ERR_CODE;
    }
  }

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    ret = pthread_join(workers[i], &tmp);
    if (ret != 0) {
      fprintf(stderr, "pthread_join(3): Could not join with a thread\n");
      return MY_ERR_CODE;
    }

    sum.found += results[i].found;
    sum.same += results[i].same;
    sum.different += results[i].different;
    sum.deficient += results[i].deficient;
    sum.failed += results[i].failed;
    sum.skipped += results[i].skipped;
  }

  printf("%d keys were found\n", sum.found);
  printf("%d keys were same\n", sum.same);
  printf("%d keys were different\n", sum.different);
  printf("%d keys were deficient\n", sum.deficient);
  printf("%d keys were failed\n", sum.failed);
  printf("%d keys were skipped\n", sum.skipped);

  for (i = 0; i < MAX_CONCURRENCY; ++i) {
    if (freeClusterState(args[i].src) == MY_ERR_CODE) return MY_ERR_CODE;
    if (freeClusterState(args[i].dest) == MY_ERR_CODE) return MY_ERR_CODE;
    free(args[i].src);
    free(args[i].dest);
  }

  return MY_OK_CODE;
}

int main(int argc, char **argv) {
  int ret;
  struct sigaction sa;
  Cluster src, dest;

  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Usage: bin/diff src-host:port dest-host:port\n");
    exit(1);
  }

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &sa, NULL) < 0) {
    fprintf(stderr, "sigaction(2): SIGPIPE failed\n");
    exit(1);
  }

  ret = fetchClusterState(argv[1], &src);
  ASSERT(ret);

  ret = fetchClusterState(argv[2], &dest);
  ASSERT(ret);

  ret = takeDiff(&src, &dest);
  ASSERT(ret);

  ret = freeClusterState(&src);
  ASSERT(ret);

  ret = freeClusterState(&dest);
  ASSERT(ret);

  exit(0);
}
