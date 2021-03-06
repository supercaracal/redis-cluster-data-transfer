#include <string.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"
#include "./cluster.h"
#include "./copy.h"

#ifndef PIPELINING_SIZE
#define PIPELINING_SIZE 10
#endif  // PIPELINING_SIZE

#define ASSERT_RESTORE_DATA(cond, msg) do {\
  if (!(cond)) {\
    fprintf(stderr, "RESTORE: %s\n", msg);\
    exit(1);\
  }\
} while (0)

typedef struct { char buf[MAX_CMD_SIZE * PIPELINING_SIZE]; int i, cnt; } Pipeline;

static inline void countRestoreResult(const Reply *reply, MigrationResult *result) {
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
        ASSERT_RESTORE_DATA(0, reply->lines[i]);
        break;
    }
  }
}

static inline void appendRestoreCmd(Pipeline *pip,
    const char *key, int keySize,
    const char *payload, int payloadSize,
    const char *ttl, int ttlSize) {
  int i, chunkSize;

  chunkSize = 32;
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "*5\r\n");

  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$7\r\n");
  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "RESTORE\r\n");

  pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", keySize);
  for (i = 0; i < keySize; ++i) pip->buf[pip->i++] = key[i];
  pip->buf[pip->i++] = '\r';
  pip->buf[pip->i++] = '\n';

  if (strncmp(ttl, "-1", 2) == 0) {
    pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$1\r\n");
    pip->i += snprintf(&pip->buf[pip->i], chunkSize, "0\r\n");
  } else {
    pip->i += snprintf(&pip->buf[pip->i], chunkSize, "$%d\r\n", ttlSize);
    for (i = 0; i < ttlSize; ++i) pip->buf[pip->i++] = ttl[i];
    pip->buf[pip->i++] = '\r';
    pip->buf[pip->i++] = '\n';
  }

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
  int i, ret;

  for (i = pip.cnt = pip.i = 0; i < keyPayloads->i; i += 3) {
    ASSERT_RESTORE_DATA((keyPayloads->types[i] == RAW), "must be a key");
    ASSERT_RESTORE_DATA((keyPayloads->types[i+1] == INTEGER), "must be a ttl msec");

    if (keyPayloads->types[i+2] == NIL || strncmp(keyPayloads->lines[i+1], "-2", 2) == 0) {
      result->skipped++;
    } else {
      ASSERT_RESTORE_DATA((keyPayloads->types[i+2] == RAW), "must be a payload");
      appendRestoreCmd(&pip,
          keyPayloads->lines[i], keyPayloads->sizes[i],
          keyPayloads->lines[i+2], keyPayloads->sizes[i+2],
          keyPayloads->lines[i+1], keyPayloads->sizes[i+1]);
    }

    if (pip.cnt % PIPELINING_SIZE > 0 && i + 3 < keyPayloads->i) continue;
    if (pip.cnt == 0) continue;

    ret = commandWithRawData(c, pip.buf, pip.i, &reply, pip.cnt);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      ASSERT_RESTORE_DATA((reply.i == pip.cnt), "lack or too much reply lines");
      countRestoreResult(&reply, result);
    }

    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

static void fetchAndTransferKeys(Conn *src, Conn *dest, const Reply *keys, MigrationResult *result) {
  Pipeline pip;
  Reply reply;
  int i, ret;

  for (i = pip.i = pip.cnt = 0; i < keys->i; ++i) {
    pip.i += snprintf(&pip.buf[pip.i], MAX_KEY_SIZE * 3,
        "ECHO %s\r\nPTTL %s\r\nDUMP %s\r\n", keys->lines[i], keys->lines[i], keys->lines[i]);
    pip.cnt++;

    if (pip.cnt % PIPELINING_SIZE > 0 && i + 1 < keys->i) continue;
    if (pip.cnt == 0) continue;

    ret = commandWithRawData(src, pip.buf, pip.i, &reply, pip.cnt * 3);
    if (ret == MY_ERR_CODE) {
      result->failed += pip.cnt;
    } else {
      ASSERT_RESTORE_DATA((reply.i == pip.cnt * 3), "key and payload pairs are wrong");
      transferKeys(dest, &reply, result);
    }

    pip.cnt = pip.i = 0;
    freeReply(&reply);
  }
}

int copyKeys(Conn *src, Conn *dest, int slot, MigrationResult *result, int dryRun) {
  char buf[MAX_CMD_SIZE];
  int ret, cnt;
  Reply reply;

  ret = countKeysInSlot(src, slot);
  if (ret == MY_ERR_CODE) return ret;
  if (ret == 0) return MY_OK_CODE;
  result->found += ret;
  if (dryRun) return MY_OK_CODE;

  cnt = ret;
  snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", slot, ret);
  ret = command(src, buf, &reply);
  if (ret == MY_ERR_CODE) {
    result->failed += cnt;
    freeReply(&reply);
    return ret;
  }

  fetchAndTransferKeys(src, dest, &reply, result);
  freeReply(&reply);

  return MY_OK_CODE;
}
