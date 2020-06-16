#include <string.h>
#include "./generic.h"
#include "./command.h"

#define MAX_REPLY_LINE_SIZE (1 << 13)

static int tryToWriteString(Conn *conn, const char *str) {
  if (fputs(str, conn->fw) == EOF) {
    fprintf(stderr, "fputs(3): %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  if (fflush(conn->fw) == EOF) {
    fprintf(stderr, "fflush(3): %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  return MY_OK_CODE;
}

static int tryToReadString(Conn *conn, char *buf, int size) {
  if (fgets(buf, size + 3, conn->fr) == NULL) {  // \r\n\0
    fprintf(stderr, "fgets(3): returns NULL: %s:%s\n", conn->addr.host, conn->addr.port);
    return MY_ERR_CODE;
  }
  return MY_OK_CODE;
}

static inline int copyReplyLineWithoutMeta(Reply *reply, const char *buf, int offset, ReplyType t) {
  int len, realLen;

  len = realLen = strlen(buf);
  if (buf[len - 1] == '\n') --len;
  if (buf[len - 1] == '\r') --len;
  len -= offset;  // special chars
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * (len + 1));  // terminator
  ASSERT_MALLOC(reply->lines[reply->i], "for new reply line");
  strncpy(reply->lines[reply->i], buf + offset, len + 1);  // terminator
  reply->lines[reply->i][len] = '\0';
  reply->types[reply->i] = t;
  reply->sizes[reply->i] = len;
  ADVANCE_REPLY_LINE(reply);
  return realLen;
}

static int parseBulkStringLength(const char *buf, Reply *reply) {
  int i = 0;

  reply->nextIdxOfLastLine = atoi(buf);

  if (reply->nextIdxOfLastLine >= 0) {
    reply->types[reply->i] = TMPBULKSTR;
    ++i;
  } else if (reply->nextIdxOfLastLine == -1) {
    ADD_NULL_REPLY(reply);
  } else {
    fprintf(stderr, "Not expected bulk string size: %d\n", reply->nextIdxOfLastLine);
    exit(1);
  }

  return i;
}

static int parseBulkString(const char *buf, Reply *reply) {
  int i = 0;

  // bulk string
  reply->nextIdxOfLastLine -= copyReplyLineWithoutMeta(reply, buf, 0, STRING);
  if (reply->nextIdxOfLastLine > 0) {
    // multi line (e.g. CLUSTER NODES, INFO, ...)
    ++i;
  }

  return i;
}

// @see https://redis.io/topics/protocol Redis Protocol specification
static int parseReplyLine(const char *buf, Reply *reply) {
  int i = 0;

  if (reply->types[reply->i] == UNKNOWN) {
    switch (buf[0]) {
      case '+':
        copyReplyLineWithoutMeta(reply, buf, 1, STRING);
        break;
      case '-':
        copyReplyLineWithoutMeta(reply, buf, 1, ERR);
        break;
      case ':':
        copyReplyLineWithoutMeta(reply, buf, 1, INTEGER);
        break;
      case '$':
        i += parseBulkStringLength(buf + 1, reply);
        break;
      case '*':
        i += atoi(buf + 1);
        break;
      default:
        ++i;  // Not expected token, Skip last remained reply line
        break;
    }
  } else if (reply->types[reply->i] == TMPBULKSTR) {
    i += parseBulkString(buf, reply);
  } else {
    fprintf(stderr, "Not expected reply line: %s: %s\n", getReplyTypeCode(reply->types[reply->i]), buf);
    exit(1);
  }

  return i;
}

static int readAndParseReply(Conn *conn, Reply *reply, int n) {
  char buf[MAX_REPLY_LINE_SIZE];
  int i, size, ret;

  INIT_REPLY(reply);

  for (i = n, ret = MY_OK_CODE; i > 0; --i) {
    size = reply->types[reply->i] == TMPBULKSTR ? reply->sizes[reply->i] : DEFAULT_REPLY_SIZE;
    ret = tryToReadString(conn, buf, size);
    if (ret == MY_ERR_CODE) break;

    ret = parseReplyLine(buf, reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  if (ret == MY_ERR_CODE) freeReply(reply);

  return ret;
}

int command(Conn *conn, const char *cmd, Reply *reply) {
  char buf[MAX_CMD_SIZE];
  int ret;

  snprintf(buf, MAX_CMD_SIZE, "%s\r\n", cmd);

  ret = tryToWriteString(conn, buf);
  if (ret == MY_ERR_CODE) return ret;

  return readAndParseReply(conn, reply, 1);
}


int pipeline(Conn *conn, const char *cmd, Reply *reply, int n) {
  int ret;

  ret = tryToWriteString(conn, cmd);
  if (ret == MY_ERR_CODE) return ret;

  return readAndParseReply(conn, reply, n);
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

void printReplyLines(const Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) {
    switch (reply->types[i]) {
      case STRING:
      case INTEGER:
      case ERR:
        printf("%s\n",  reply->lines[i]);
        break;
      case RAW:
        PRINT_MIXED_BINARY(reply->lines[i], reply->sizes[i]);
        printf("\n");
        break;
      case NIL:
        printf("(null)\n");
        break;
      default:
        break;
    }
  }
}

char *getReplyTypeCode(ReplyType r) {
  switch (r) {
    case STRING: return "STRING";
    case INTEGER: return "INTEGER";
    case RAW: return "RAW";
    case ERR: return "ERR";
    case NIL: return "NIL";
    case TMPBULKSTR: return "TMPBULKSTR";
    case TMPARR: return "TMPARR";
    default: return "UNKNOWN";
  }
}

#ifdef TEST
int PublicForTestParseReplyLine(const char *buf, Reply *reply) {
  return parseReplyLine(buf, reply);
}
#endif
