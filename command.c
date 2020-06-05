#include <string.h>
#include "./command.h"
#include "./net.h"

#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->err = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
  ASSERT_MALLOC(r->lines, "for init reply lines");\
  r->types = (ReplyType *) malloc(sizeof(ReplyType) * r->size);\
  ASSERT_MALLOC(r->types, "for init reply types");\
} while (0)

#define EXPAND_REPLY_IF_NEEDED(r) do {\
  if (r->i == r->size) {\
    void *tmp;\
    r->size *= 2;\
    tmp = realloc(r->lines, sizeof(char *) * r->size);\
    ASSERT_REALLOC(tmp, "for reply lines");\
    r->lines = (char **) tmp;\
    tmp = realloc(r->types, sizeof(ReplyType) * r->size);\
    ASSERT_REALLOC(tmp, "for reply types");\
    r->types = (ReplyType *) tmp;\
  }\
} while (0)

static inline int copyReplyLineWithoutMeta(Reply *reply, const char *buf, int offset, ReplyType t) {
  int len, realLen;

  len = realLen = strlen(buf);
  if (buf[len - 1] == '\n') --len;
  if (buf[len - 1] == '\r') --len;
  len -= offset; // special chars
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * (len + 1)); // terminator
  ASSERT_MALLOC(reply->lines[reply->i], "for new reply lines");
  strncpy(reply->lines[reply->i], buf + offset, len + 1); // terminator
  reply->lines[reply->i][len] = '\0';
  reply->types[reply->i] = t;
  reply->i++;
  return realLen;
}

static int executeCommand(Conn *conn, const char *cmd, Reply *reply, int n) {
  int i, size, readSize;
  char *buf;

  if (n == 1) {
    size = strlen(cmd);
    buf = (char *) malloc(sizeof(char) * (size + 3)); // \r \n \0
    ASSERT_MALLOC(buf, "for writing command buffer");
    snprintf(buf, size + 3, "%s\r\n", cmd);
    size = fputs(buf, conn->fw);
    free(buf);
  } else {
    size = fputs(cmd, conn->fw);
  }
  if (size == EOF) {
    fprintf(stderr, "fputs(3): %s to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  if (fflush(conn->fw) == EOF) {
    fprintf(stderr, "fflush(3): %s to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  INIT_REPLY(reply);
  for (i = n, size = DEFAULT_REPLY_SIZE; i > 0; --i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    buf = (char *) malloc(sizeof(char) * (size + 3)); // \r \n \0
    ASSERT_MALLOC(buf, "for reading reply buffer");
    if (fgets(buf, size + 3, conn->fr) == NULL) { // \r \n \0
      free(buf);
      freeReply(reply);
      if (n > 1) {
        fprintf(stderr, "fgets(3): returns NULL when execute pipelined commands to %s:%s\n%s", conn->addr.host, conn->addr.port, cmd);
        fprintf(stderr, "Reconnect to %s:%s for retrying the above commands\n", conn->addr.host, conn->addr.port);
      } else {
        fprintf(stderr, "fgets(3): returns NULL when execute `%s` to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
        fprintf(stderr, "Reconnect to %s:%s for retrying `%s`\n", conn->addr.host, conn->addr.port, cmd);
      }
      return reconnect(conn) == MY_OK_CODE ? executeCommand(conn, cmd, reply, n) : MY_ERR_CODE;
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

  if (n == 1 && reply->err) {
    fprintf(stderr, "Tried `%s` to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    fprintf(stderr, "%s\n", LAST_LINE(reply));
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

int command(Conn *conn, const char *cmd, Reply *reply) {
  return executeCommand(conn, cmd, reply, 1);
}


int pipeline(Conn *conn, const char *cmd, Reply *reply, int n) {
  return executeCommand(conn, cmd, reply, n);
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

int isKeylessCommand(const char *cmd) {
  while (*cmd != '\0') {
    if (*cmd == ' ') return 0;
    ++cmd;
  }
  return 1;
}
