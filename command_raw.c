#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

#define MAX_RECV_SIZE (1 << 12)

#define ASSERT_SENT_SIZE(expected, actual) do {\
  if ((expected) != (actual)) {\
    fprintf(stderr, "Failed to send command: %d / %d\n", actual, expected);\
    exit(1);\
  }\
} while (0)

#define ASSERT_REPLY_PARSE(cond, msg) do {\
  if (!(cond)) {\
    fprintf(stderr, "Failed to parse for reply: %s\n", (msg));\
    exit(1);\
  }\
} while (0)

#define ASSERT_CMD_SIZE(size) do {\
  if ((size) < 1) {\
    fprintf(stderr, "Empty command was given\n");\
    exit(1);\
  }\
} while (0)

#define ASSERT_REPLY_PTR(reply) do {\
  if ((reply) == NULL) {\
    fprintf(stderr, "NULL reply pointer was given\n");\
    exit(1);\
  } else if ((reply)->size < 1) {\
    fprintf(stderr, "fishy reply pointer was given\n");\
    exit(1);\
  }\
} while (0)

static inline char *errNo2Code(int n) {
  switch (n) {
    case  4: return "EINTR";
    case 11: return "EAGAIN";
    case 32: return "EPIPE";
    default: return "!!!!!";
  }
}

static int tryToWriteToSocket(Conn *conn, const void *buf, int size) {
  int sock, ret;

  fflush(conn->fw);
  sock = fileno(conn->fw);

  ret = send(sock, buf, size, 0);
  if (ret == -1) {
    ret = errno;
    perror("send(2)");
    fprintf(stderr, "send(2): %s: %s:%s\n", errNo2Code(ret), conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  return ret;
}

static int tryToReadFromSocket(Conn *conn, void *buf, int size) {
  int sock, ret;

  sock = fileno(conn->fr);

  ret = recv(sock, buf, size, 0);
  if (ret == -1) {
    ret = errno;
    perror("recv(2)");
    fprintf(stderr, "recv(2): %s: %s:%s\n", errNo2Code(ret), conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  if (ret == 0) {
    fprintf(stderr, "recv(2): returns 0: %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }

  return ret;
}

static inline void finalizeSimpleString(Reply *reply) {
  int n;

  switch (reply->types[reply->i]) {
    case STRING:
    case ERR:
    case INTEGER:
      reply->lines[reply->i][reply->nextIdxOfLastLine] = '\0';
      ADVANCE_REPLY_LINE(reply);
      break;
    case TMPBULKSTR:
      reply->lines[reply->i][reply->nextIdxOfLastLine] = '\0';
      n = atoi(reply->lines[reply->i]);
      if (n >= 0) {
        free(reply->lines[reply->i]);
        INIT_REPLY_LINE(reply);
        reply->sizes[reply->i] = n;
        reply->types[reply->i] = RAW;
      } else if (n == -1) {
        ADD_NULL_REPLY(reply);
      } else {
        ASSERT_REPLY_PARSE(0, "not expected negative size for bulk string");
      }
      break;
    case TMPARR:
      // TODO(me): implement here if needed for treating nested array
      free(reply->lines[reply->i]);
      INIT_REPLY_LINE(reply);
      break;
    default:
      ASSERT_REPLY_PARSE(0, "failed to finalize as simple string");
      break;
  }
}

static inline void parseSimpleStringChar(Reply *reply, char c) {
  int isSimpleStrType =
    reply->types[reply->i] == STRING ||
    reply->types[reply->i] == ERR ||
    reply->types[reply->i] == INTEGER;

  int isMetaType =
    reply->types[reply->i] == TMPBULKSTR ||
    reply->types[reply->i] == TMPARR;

  ASSERT_REPLY_PARSE((isSimpleStrType || isMetaType), "could not treat as a simple string");

  if (reply->lines[reply->i] == NULL) {
    reply->lines[reply->i] = (char *) malloc(sizeof(char) * DEFAULT_REPLY_SIZE);
    ASSERT_MALLOC(reply->lines[reply->i], "for new reply line of simple string");
  }

  switch (c) {
    case '\r':
      // skip
      break;
    case '\n':
      finalizeSimpleString(reply);
      break;
    default:
      reply->lines[reply->i][reply->nextIdxOfLastLine] = c;
      reply->nextIdxOfLastLine++;
      reply->sizes[reply->i]++;
      break;
  }
}

static inline void parseBulkStringAsBinary(Reply *reply, char c) {
  ASSERT_REPLY_PARSE((reply->types[reply->i] == RAW), "not raw type");

  if (reply->nextIdxOfLastLine == reply->sizes[reply->i]) {
    if (c == '\r') return;  // skip
    if (c == '\n') {
      // completed
      ADVANCE_REPLY_LINE(reply);
      return;
    }
    ASSERT_REPLY_PARSE(0, "could not parse as a bulk string");
  }

  if (reply->lines[reply->i] == NULL) {
    reply->lines[reply->i] = (char *) malloc(sizeof(char) * reply->sizes[reply->i]);
    ASSERT_MALLOC(reply->lines[reply->i], "for new reply of binary");
  }

  reply->lines[reply->i][reply->nextIdxOfLastLine] = c;
  reply->nextIdxOfLastLine++;
}

static int parseRawReply(const char *buf, int size, Reply *reply) {
  int i;

  for (i = 0; i < size; ++i) {
    if (reply->types[reply->i] == UNKNOWN) {
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
          reply->types[reply->i] = TMPBULKSTR;
          break;
        case '*':
          reply->types[reply->i] = TMPARR;
          break;
        case '\r':
        case '\n':
          // skip
          break;
        default:
          PRINT_MIXED_BINARY(buf, size);
          printf("\n");
          ASSERT_REPLY_PARSE(0, "not expected token");
          break;
      }
    } else if (reply->types[reply->i] == RAW) {
      parseBulkStringAsBinary(reply, buf[i]);
    } else {
      parseSimpleStringChar(reply, buf[i]);
    }
  }

  return reply->types[reply->i] == UNKNOWN ? MY_OK_CODE : NEED_MORE_REPLY;
}

int commandWithRawData(Conn *conn, const void *cmd, int size, Reply *reply, int expectedNumberOfLines) {
  int ret;

  ASSERT_CMD_SIZE(size);

  ret = tryToWriteToSocket(conn, cmd, size);
  if (ret == MY_ERR_CODE) return ret;
  ASSERT_SENT_SIZE(size, ret);

  INIT_REPLY(reply);
  return readRemainedReplyLines(conn, reply, expectedNumberOfLines);
}

int readRemainedReplyLines(Conn *conn, Reply *reply, int expectedNumberOfLines) {
  int ret;
  char buf[MAX_RECV_SIZE];

  ASSERT_REPLY_PTR(reply);

  ret = NEED_MORE_REPLY;
  while (ret == NEED_MORE_REPLY || reply->i < expectedNumberOfLines) {
    ret = tryToReadFromSocket(conn, buf, MAX_RECV_SIZE);
    if (ret == MY_ERR_CODE) break;

    ret = parseRawReply(buf, ret, reply);
  }

  return ret;
}

#ifdef TEST
int PublicForTestParseRawReply(const char *buf, int size, Reply *reply) {
  return parseRawReply(buf, size, reply);
}
#endif
