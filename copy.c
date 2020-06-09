#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"
#include "./copy.h"

#ifndef PIPELINING_SIZE
#define PIPELINING_SIZE 10
#endif // PIPELINING_SIZE

typedef struct { char buf[MAX_CMD_SIZE * PIPELINING_SIZE]; int i, cnt; } Pipeline;

static inline void countRestoreResult(const Reply *reply, MigrationResult *result, int expected) {
  int i;

  for (i = 0; i < reply->i; ++i) {
    switch (reply->types[i]) {
      case STRING:
        result->copied++;
        break;
      case ERR:
        fprintf(stderr, "%s\n", reply->lines[i]);
        result->failed++;
        break;
      default:
        result->failed++;
        break;
    }
  }

  if (reply->i < expected) result->failed += expected - reply->i;
}

static inline void appendRestoreCmd(Pipeline *pip, const char *key, int keySize, const char *payload, int payloadSize) {
  int j;

  pip->i += snprintf(&pip->buf[pip->i], 12, "*5\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "RESTORE\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "$%d\r\n", keySize);
  pip->i += snprintf(&pip->buf[pip->i], keySize + 3, "%s\r\n", key);
  pip->i += snprintf(&pip->buf[pip->i], 12, "$1\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "0\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "$%d\r\n", payloadSize);
  for (j = 0; j < payloadSize; ++j) pip->buf[pip->i++] = payload[j];
  pip->buf[pip->i++] = '\r';
  pip->buf[pip->i++] = '\n';
  pip->i += snprintf(&pip->buf[pip->i], 12, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], 12, "REPLACE\r\n");
  pip->cnt++;
}

static inline void sendResotreCmd(Conn *c, Pipeline *pip, Reply *reply, MigrationResult *result) {
  pip->buf[pip->i] = '\0';
  commandWithRawData(c, pip->buf, reply, pip->i);
  countRestoreResult(reply, result, pip->cnt);
  pip->cnt = pip->i = 0;
}

static void transferKeys(Conn *c, const Reply *keys, int first, int size, const Reply *values, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i;

  for (i = pip.cnt = pip.i = 0; i < size; ++i) {
    if (values->types[i] == NIL) {
      result->skipped++;
      continue;
    }

    appendRestoreCmd(&pip, keys->lines[first+i], keys->sizes[first+i], values->lines[i], values->sizes[i]);
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    sendResotreCmd(c, &pip, &reply, result);
    freeReply(&reply);
  }
}

static void copyKeys(Conn *src, Conn *dest, const Reply *keys, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret, first;

  for (i = first = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2, "DUMP %s\r\n", keys->lines[i]);
    pip.cnt++;
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    pip.buf[pip.i] = '\0';
    ret = commandWithRawData(src, pip.buf, &reply, pip.i);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      if (pip.cnt != reply.i) {
        // fprintf(stderr, "%s\n", pip.buf);
        // printReplyLines(&reply);
        fprintf(stderr, "RESTORE: keys and dump values are mismatched\n");
        exit(1);
      }
      transferKeys(dest, keys, first, pip.cnt, &reply, result);
    }
    pip.cnt = pip.i = 0;
    first = i + 1;
    freeReply(&reply);
  }
}

int migrateKeys(const Cluster *src, const Cluster *dest, int slot, int dryRun, MigrationResult *result) {
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
