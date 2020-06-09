#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"
#include "./copy.h"

#ifndef PIPELINING_SIZE
#define PIPELINING_SIZE 10
#endif  // PIPELINING_SIZE

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
        result->skipped++;  // TODO(me): legit?
        break;
    }
  }

  if (reply->i < expected) result->failed += expected - reply->i;
}

static inline void appendRestoreCmd(Pipeline *pip, const char *key, int keySize, const char *payload, int payloadSize) {
  int j, chunkSize;

  chunkSize = 12;
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "*5\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "RESTORE\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", keySize);
  pip->i += snprintf(&pip->buf[pip->i], keySize + 3, "%s\r\n", key);
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$1\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "0\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", payloadSize);
  for (j = 0; j < payloadSize; ++j) pip->buf[pip->i++] = payload[j];
  pip->buf[pip->i++] = '\r';
  pip->buf[pip->i++] = '\n';
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "REPLACE\r\n");
  pip->cnt++;
}

static void transferKeys(Conn *c, const Reply *keyValues, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i;

  if (keyValues->i % 2 > 0) {
    printReplyLines(keyValues);
    fprintf(stderr, "RESTORE: Keys and payloads were mismatched.");
    exit(1);
  }

  for (i = pip.cnt = pip.i = 0; i < keyValues->i; i += 2) {
    appendRestoreCmd(&pip, keyValues->lines[i], keyValues->sizes[i], keyValues->lines[i+1], keyValues->sizes[i+1]);
    if (i > 0 && i % PIPELINING_SIZE > 0 && i < keyValues->i - 1) continue;

    pip.buf[pip.i] = '\0';
    commandWithRawData(c, pip.buf, &reply, pip.i);
    countRestoreResult(&reply, result, pip.cnt);
    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

static void copyKeys(Conn *src, Conn *dest, const Reply *keys, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret;

  for (i = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2, "ECHO %s\r\nDUMP %s\r\n", keys->lines[i], keys->lines[i]);
    pip.cnt++;
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

    pip.buf[pip.i] = '\0';
    ret = commandWithRawData(src, pip.buf, &reply, pip.i);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      transferKeys(dest, &reply, result);
    }
    pip.cnt = pip.i = 0;
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
