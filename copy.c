#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"
#include "./cluster.h"
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
        result->skipped++;  // FIXME(me): legit?
        break;
    }
  }

  if (reply->i < expected) result->failed += expected - reply->i;
}

static inline void appendRestoreCmd(Pipeline *pip, const char *key, int keySize, const char *payload, int payloadSize) {
  int i, chunkSize;

  chunkSize = 12;
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "*5\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "RESTORE\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", keySize);
  for (i = 0; i < keySize; ++i) pip->buf[pip->i++] = key[i];
  pip->buf[pip->i++] = '\r';
  pip->buf[pip->i++] = '\n';
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$1\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "%d\r\n", 0);  // TTL msec
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", payloadSize);
  for (i = 0; i < payloadSize; ++i) pip->buf[pip->i++] = payload[i];
  pip->buf[pip->i++] = '\r';
  pip->buf[pip->i++] = '\n';
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "REPLACE\r\n");
  pip->cnt++;
}

static void transferKeys(Conn *c, const Reply *keyPayloads, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i;

  if (keyPayloads->i % 2 > 0) {
    fprintf(stderr, "RESTORE: Keys and payloads were mismatched\n");
    exit(1);
  }

  for (i = pip.cnt = pip.i = 0; i < keyPayloads->i; i += 2) {
    if (keyPayloads->types[i+1] != RAW) {
      result->skipped++;
      continue;
    }

    appendRestoreCmd(&pip, keyPayloads->lines[i], keyPayloads->sizes[i], keyPayloads->lines[i+1], keyPayloads->sizes[i+1]);
    if (pip.cnt % PIPELINING_SIZE > 0 && i + 2 < keyPayloads->i) continue;

    commandWithRawData(c, pip.buf, &reply, pip.i);
    countRestoreResult(&reply, result, pip.cnt);

    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

static void fetchAndTransferKeys(Conn *src, Conn *dest, const Reply *keys, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret;

  for (i = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 2, "ECHO %s\r\nDUMP %s\r\n", keys->lines[i], keys->lines[i]);
    pip.cnt++;
    if ((i + 1) % PIPELINING_SIZE != 0 && i < keys->i - 1) continue;

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

int copyKeys(Conn *src, Conn *dest, int slot, MigrationResult *result, int dryRun) {
  char buf[MAX_CMD_SIZE];
  int ret;
  Reply reply;

  ret = countKeysInSlot(src, slot);
  if (ret == MY_ERR_CODE) return ret;
  if (ret == 0) return MY_OK_CODE;
  result->found += ret;
  if (dryRun) return MY_OK_CODE;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", slot, ret);
  ret = command(src, buf, &reply);
  if (ret == MY_ERR_CODE) {
    freeReply(&reply);
    return ret;
  }

  fetchAndTransferKeys(src, dest, &reply, result);
  freeReply(&reply);

  return MY_OK_CODE;
}
