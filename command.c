#include <string.h>
#include "./command.h"

#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->err = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
  r->types = (ReplyType *) malloc(sizeof(ReplyType) * r->size);\
} while (0)

#define EXPAND_REPLY_IF_NEEDED(r) do {\
  if (r->i == r->size) {\
    r->size *= 2;\
    r->lines = (char **) realloc(r->lines, sizeof(char *) * r->size);\
    r->types = (ReplyType *) realloc(r->types, sizeof(ReplyType) * r->size);\
  }\
} while (0)

static inline int copyReplyLineWithoutMeta(Reply *reply, const char *buf, int offset, ReplyType t) {
  int len, realLen;

  len = realLen = strlen(buf);
  if (buf[len - 1] == '\n') --len;
  if (buf[len - 1] == '\r') --len;
  len -= offset; // special chars
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * (len + 1)); // terminator
  strncpy(reply->lines[reply->i], buf + offset, len + 1); // terminator
  reply->lines[reply->i][len] = '\0';
  reply->types[reply->i] = t;
  reply->i++;
  return realLen;
}

int command(const Conn *conn, const char *cmd, Reply *reply) {
  int i, size, readSize;
  char *buf;

  size = strlen(cmd);
  buf = (char *) malloc(sizeof(char) * (size + 3)); // \r \n \0
  snprintf(buf, size + 3, "%s\r\n", cmd);
  if (fputs(buf, conn->fw) == EOF) {
    fprintf(stderr, "fputs(3): %s to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  if (fflush(conn->fw) == EOF) {
    fprintf(stderr, "fflush(3): %s to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  free(buf);

  INIT_REPLY(reply);
  for (i = 1, size = DEFAULT_REPLY_SIZE; i > 0; --i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    buf = (char *) malloc(sizeof(char) * (size + 3)); // \r \n \0
    if (fgets(buf, size + 3, conn->fr) == NULL) { // \r \n \0
      free(buf);
      fprintf(stderr, "fgets(3): returns NULL when execute `%s` to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
      return MY_ERR_CODE;
    }
    // @see https://redis.io/topics/protocol Redis Protocol specification
    switch (buf[0]) {
      case '+':
        copyReplyLineWithoutMeta(reply, buf, 1, STRING);
        break;
      case '-':
        copyReplyLineWithoutMeta(reply, buf, 1, STRING);
        reply->err = 1;
        break;
      case ':':
        copyReplyLineWithoutMeta(reply, buf, 1, INTEGER);
        break;
      case '$':
        size = atoi(buf + 1);
        if (size > -1) ++i;
        break;
      case '*':
        i += atoi(buf + 1);
        break;
      default:
        // bulk string
        readSize = copyReplyLineWithoutMeta(reply, buf, 0, STRING);
        if (readSize < size) {
          // multi line (e.g. CLUSTER NODES, INFO, ...)
          size -= readSize;
          if (size > 0) ++i;
        } else {
          size = DEFAULT_REPLY_SIZE;
        }
        break;
    }
    free(buf);
  }

  if (reply->err) {
    fprintf(stderr, "Tried `%s` to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    fprintf(stderr, "Error: %s\n", LAST_LINE(reply));
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

void freeReply(Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) free(reply->lines[i]);
  free(reply->lines);
  reply->lines = NULL;
  free(reply->types);
  reply->types = NULL;
  reply->i = reply->size = reply->err = 0;
}

void printReplyLines(const Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) fprintf(stdout, "%s\n", reply->lines[i]);
}
