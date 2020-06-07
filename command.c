#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "./command.h"
#include "./net.h"

#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256
#define MAX_RECV_SIZE (1 << 14)

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
  ASSERT_MALLOC(r->lines, "for init reply lines");\
  r->types = (ReplyType *) malloc(sizeof(ReplyType) * r->size);\
  ASSERT_MALLOC(r->types, "for init reply types");\
  r->sizes = (int *) malloc(sizeof(int) * r->size);\
  ASSERT_MALLOC(r->sizes, "for init reply sizes");\
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
    tmp = realloc(r->sizes, sizeof(int) * r->size);\
    ASSERT_REALLOC(tmp, "for reply sizes");\
    r->sizes = (int *) tmp;\
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
  reply->sizes[reply->i] = realLen;
  reply->i++;
  return realLen;
}

static inline void copyReplyLineRaw(Reply *reply, const char *buf, int size) {
  int i;

  reply->lines[reply->i] = (char *) malloc(sizeof(char) * size);
  ASSERT_MALLOC(reply->lines[reply->i], "for new reply raw lines");
  for (i = 0; i < size; ++i) reply->lines[reply->i][i] = buf[i];
  reply->types[reply->i] = RAW;
  reply->sizes[reply->i] = size;
  reply->i++;
}

static inline void addNullReply(Reply *reply) {
  reply->lines[reply->i] = NULL;
  reply->types[reply->i] = NIL;
  reply->sizes[reply->i] = 0;
  reply->i++;
}

static int executeCommand(Conn *conn, const char *cmd, Reply *reply, int n) {
  int i, ret, size, readSize;
  char *buf;

  size = fputs(cmd, conn->fw);
  if (size == EOF) {
    fprintf(stderr, "fputs(3): %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  if (fflush(conn->fw) == EOF) {
    fprintf(stderr, "fflush(3): %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  INIT_REPLY(reply);
  for (i = n, size = DEFAULT_REPLY_SIZE, ret = MY_OK_CODE; i > 0; --i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    buf = (char *) malloc(sizeof(char) * (size + 3)); // \r \n \0
    ASSERT_MALLOC(buf, "for reading reply buffer");
    if (fgets(buf, size + 3, conn->fr) == NULL) { // \r \n \0
      free(buf);
      freeReply(reply);
      fprintf(stderr, "fgets(3): returns NULL, trying to reconnect to %s:%s\n", conn->addr.host, conn->addr.port);
      return reconnect(conn) == MY_OK_CODE ? executeCommand(conn, cmd, reply, n) : MY_ERR_CODE;
    }
    // @see https://redis.io/topics/protocol Redis Protocol specification
    switch (buf[0]) {
      case '+':
        copyReplyLineWithoutMeta(reply, buf, 1, STRING);
        break;
      case '-':
        copyReplyLineWithoutMeta(reply, buf, 1, ERR);
        fprintf(stderr, "%s:%s says %s\n", conn->addr.host, conn->addr.port, LAST_LINE(reply));
        ret = MY_ERR_CODE;
        break;
      case ':':
        copyReplyLineWithoutMeta(reply, buf, 1, INTEGER);
        break;
      case '$':
        size = atoi(buf + 1);
        if (size >= 0) {
          ++i;
        } else if (size == -1) {
          addNullReply(reply);
        }
        break;
      case '*':
        i += atoi(buf + 1);
        break;
      default:
        if (buf[0] == '\0') {
          // DUMP
          copyReplyLineRaw(reply, buf, size);
        } else {
          // bulk string
          readSize = copyReplyLineWithoutMeta(reply, buf, 0, STRING);
          if (readSize < size) {
            // multi line (e.g. CLUSTER NODES, INFO, ...)
            size -= readSize;
            if (size > 0) ++i;
          } else {
            size = DEFAULT_REPLY_SIZE;
          }
        }
        break;
    }
    free(buf);
  }

  return ret;
}

int command(Conn *conn, const char *cmd, Reply *reply) {
  char *buf;
  int ret;

  buf = (char *) malloc(sizeof(char) * MAX_CMD_SIZE);
  ASSERT_MALLOC(buf, "for writing command buffer");
  snprintf(buf, MAX_CMD_SIZE, "%s\r\n", cmd);
  ret = executeCommand(conn, buf, reply, 1);
  free(buf);

  return ret;
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
  free(reply->sizes);
  reply->sizes = NULL;
  reply->i = reply->size = 0;
}

int commandWithRawData(Conn *conn, const void *cmd, Reply *reply, int size) {
  int sock, ret;
  char buf[MAX_RECV_SIZE];

  fflush(conn->fw);
  sock = fileno(conn->fw);

  ret = send(sock, cmd, size, 0);
  if (ret == -1) {
    fprintf(stderr, "send(2): returns -1: %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  ret = recv(sock, buf, MAX_RECV_SIZE, 0);
  if (ret == -1) {
    fprintf(stderr, "recv(2): returns -1: %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

void printReplyLines(const Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) {
    switch (reply->types[i]) {
      case STRING:
      case INTEGER:
      case ERR:
        fprintf(stdout, "%s\n",  reply->lines[i]);
        break;
      case RAW:
        fprintf(stdout, "(binary)\n");
        break;
      case NIL:
        fprintf(stdout, "(null)\n");
        break;
      default:
        break;
    }
  }
}

int isKeylessCommand(const char *cmd) {
  while (*cmd != '\0') {
    if (*cmd == ' ') return 0;
    ++cmd;
  }
  return 1;
}
