#include <sys/types.h>
#include <sys/socket.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

#define MAX_RECV_SIZE (1 << 16)

static inline int copyReplyLineFromMultiLine(Reply *reply, const char *buf, int size, ReplyType t) {
  int i;

  reply->lines[reply->i] = (char *) malloc(sizeof(char) * size);
  ASSERT_MALLOC(reply->lines[reply->i], "for new reply line from multi lines");
  for (i = 0; buf[i] != '\r' && buf[i] != '\n'; ++i) reply->lines[reply->i][i] = buf[i];
  reply->lines[reply->i][i] = '\0';
  for (; buf[i] == '\r' && buf[i] == '\n'; ++i) {}
  reply->types[reply->i] = t;
  reply->sizes[reply->i] = i;
  reply->i++;
  return i;
}

static inline int copyReplyLinesFromMultiLine(Reply *reply, const char *buf, int size) {
  int i;

  reply->sizes[reply->i] = size;
  reply->types[reply->i] = RAW;
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * (size + 1));
  ASSERT_MALLOC(reply->lines[reply->i], "for new reply lines from multi lines");
  for (i = 0; i < size; ++i) reply->lines[reply->i][i] = buf[i];
  reply->i++;
  return size;
}

static void parseRawReply(const char *buf, Reply *reply, int size) {
  int i, j, n;
  char tmp[DEFAULT_REPLY_SIZE];

  INIT_REPLY(reply);
  for (i = 0; i < size; ++i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    switch (buf[i]) {
      case '+':
        ++i;
        i += copyReplyLineFromMultiLine(reply, &buf[i], DEFAULT_REPLY_SIZE, STRING);
        break;
      case '-':
        ++i;
        i += copyReplyLineFromMultiLine(reply, &buf[i], DEFAULT_REPLY_SIZE, ERR);
        break;
      case ':':
        ++i;
        i += copyReplyLineFromMultiLine(reply, &buf[i], DEFAULT_REPLY_SIZE, INTEGER);
        break;
      case '$':
        ++i;
        for (j = 0; buf[i] != '\r'; ++j, ++i) tmp[j] = buf[i];
        tmp[j] = '\0';
        ++i;  // \n
        n = atoi(tmp);
        if (n >= 0) {
          i += copyReplyLinesFromMultiLine(reply, &buf[i + 1], n);
        } else if (n == -1) {
          ADD_NULL_REPLY(reply);
        }
        break;
      case '*':
        // TODO(me): impl
        while (buf[i] != '\r') ++i;
        ++i;  // \n
        break;
      default:
        // not expected token
        break;
    }
  }
}

int tryToReadFromSocket(Conn *conn, void *buf) {
  int sock, ret;

  sock = fileno(conn->fr);

  ret = recv(sock, buf, MAX_RECV_SIZE, 0);
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

int tryToWriteToSocket(Conn *conn, const void *buf, int size) {
  int sock, ret;

  fflush(conn->fw);
  sock = fileno(conn->fw);

  ret = send(sock, buf, size, 0);
  if (ret == -1) {
    perror("send(2)");
    return reconnect(conn) == MY_OK_CODE ? tryToWriteToSocket(conn, buf, size) : MY_ERR_CODE;
  }

  return ret;
}

int commandWithRawData(Conn *conn, const void *cmd, Reply *reply, int size) {
  int ret;
  char buf[MAX_RECV_SIZE];

  ret = tryToWriteToSocket(conn, cmd, size);
  if (ret == MY_ERR_CODE) return ret;

  ret = tryToReadFromSocket(conn, buf);
  if (ret == MY_ERR_CODE) return ret;

  parseRawReply(buf, reply, ret);

  return MY_OK_CODE;
}
