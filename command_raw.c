#include <sys/types.h>
#include <sys/socket.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

#define MAX_RECV_SIZE (1 << 12)
#define NEED_MORE_REPLY -2
#define MAX_RECONNECT_ATTEMPT 3

#define ASSERT_REPLY_PARSE(cond, msg) do {\
  if (!(cond)) {\
    fprintf(stderr, "Failed to parse for reply: %s\n", (msg));\
    exit(1);\
  }\
} while (0)

static int tryToWriteToSocket(Conn *conn, const void *buf, int size, int attempt) {
  int sock, ret;

  if (attempt <= 0) return MY_ERR_CODE;

  fflush(conn->fw);
  sock = fileno(conn->fw);

  ret = send(sock, buf, size, 0);
  if (ret == -1) {
    perror("send(2)");
    return reconnect(conn) == MY_OK_CODE ? tryToWriteToSocket(conn, buf, size, attempt - 1) : MY_ERR_CODE;
  }

  return ret;
}

static int tryToReadFromSocket(Conn *conn, void *buf, int size) {
  int sock, ret;

  sock = fileno(conn->fr);

  ret = recv(sock, buf, size, 0);
  if (ret == -1) {
    perror("recv(2)");
    return MY_ERR_CODE;
  }
  if (ret == 0) {
    fprintf(stderr, "recv(2): returns 0: %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  return ret;
}

static inline void appendSimpleStringChar(Reply *reply, char c) {
  ASSERT_REPLY_PARSE((reply->types[reply->i] == STRING ||
        reply->types[reply->i] == ERR || reply->types[reply->i] == INTEGER), "not simple string type");

  if (reply->lines[reply->i] == NULL) {
    reply->lines[reply->i] = (char *) malloc(sizeof(char) * DEFAULT_REPLY_SIZE);
    ASSERT_MALLOC(reply->lines[reply->i], "for new reply line of simple string");
  }
  if (c == '\r') return;
  reply->sizes[reply->i]++;
  if (c == '\n') {
    reply->lines[reply->i][reply->nextIdxOfLastLine] = '\0';
    ADVANCE_REPLY_LINE(reply);
    return;
  }
  reply->lines[reply->i][reply->nextIdxOfLastLine] = c;
  reply->nextIdxOfLastLine++;
}

static inline int parseBulkStringLength(const char *buf, int size, Reply *reply) {
  char tmp[DEFAULT_REPLY_SIZE];
  int i, n;

  for (i = 0; buf[i] != '\n' && i < size; ++i) tmp[i] = buf[i];
  tmp[i] = '\0';
  n = atoi(tmp);
  if (n >= 0) {
    reply->sizes[reply->i] = n;
    reply->types[reply->i] = RAW;
    if (i == size) return NEED_MORE_REPLY;
    ASSERT_REPLY_PARSE((buf[i] == '\n'), "lack of bulk string length");
  } else if (n == -1) {
    ADD_NULL_REPLY(reply);
  } else {
    ASSERT_REPLY_PARSE(0, "not expected negative size for bulk string");
  }
  return i;
}

static inline int parseBulkStringAsBinary(Reply *reply, const char *buf, int size) {
  int i;

  ASSERT_REPLY_PARSE((reply->types[reply->i] == RAW), "not raw type");

  if (reply->lines[reply->i] == NULL) {
    reply->lines[reply->i] = (char *) malloc(sizeof(char) * reply->sizes[reply->i]);
    ASSERT_MALLOC(reply->lines[reply->i], "for new reply of binary");
  }
  for (i = 0; reply->nextIdxOfLastLine < reply->sizes[reply->i] && i < size; ++i) {
    reply->lines[reply->i][reply->nextIdxOfLastLine++] = buf[i];
  }
  if (reply->nextIdxOfLastLine < reply->sizes[reply->i]) return NEED_MORE_REPLY;
  ADVANCE_REPLY_LINE(reply);
  while (buf[i] == '\r' || buf[i] == '\n') ++i;
  return i;
}

static int parseRawReply(const char *buf, int size, Reply *reply) {
  int i, ret;

  for (i = 0; i < size; ++i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    if (reply->types[reply->i] == RAW) {
      ret = parseBulkStringAsBinary(reply, &buf[i], size - i);
      if (ret == NEED_MORE_REPLY) return ret;
      i += ret;
    } else {
      switch (buf[i]) {
        case '+':
          reply->types[reply->i] = STRING;
          break;
        case '-':
          reply->types[reply->i] = ERR;
          break;
        case ':':
          reply->types[reply->i] = INTEGER;
          break;
        case '$':
          ++i;
          ret = parseBulkStringLength(&buf[i], size - i, reply);
          if (ret == NEED_MORE_REPLY) return ret;
          i += ret;
          break;
        case '*':
          // TODO(me): implement for nested array if needed
          while (buf[i] != '\n' && i < size) ++i;
          if (i == size) return NEED_MORE_REPLY;
          break;
        default:
          appendSimpleStringChar(reply, buf[i]);
          break;
      }
    }
  }
  return MY_OK_CODE;
}

int commandWithRawData(Conn *conn, const void *cmd, Reply *reply, int size) {
  int ret;

  ret = tryToWriteToSocket(conn, cmd, size, MAX_RECONNECT_ATTEMPT);
  if (ret == MY_ERR_CODE) return ret;

  INIT_REPLY(reply);
  return readRemainedReplyLines(conn, reply);
}

int readRemainedReplyLines(Conn *conn, Reply *reply) {
  int ret;
  char buf[MAX_RECV_SIZE];

  ret = tryToReadFromSocket(conn, buf, MAX_RECV_SIZE);
  if (ret == MY_ERR_CODE) return ret;

  ret = parseRawReply(buf, ret, reply);
  if (ret == NEED_MORE_REPLY) readRemainedReplyLines(conn, reply);

  return MY_OK_CODE;
}
