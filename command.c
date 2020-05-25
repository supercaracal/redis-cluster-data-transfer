#include <string.h>
#include "./command.h"

#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->err = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
} while (0)

#define EXPAND_REPLY_IF_NEEDED(r) do {\
  if (r->i == r->size) {\
    r->size *= 2;\
    r->lines = (char **) realloc(r->lines, sizeof(char *) * r->size);\
  }\
} while (0)

#define COPY_REPLY_LINE_WITHOUT_META(reply, buf, offset) do {\
  int len = strlen(buf) + 1 - 2 - offset;\
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * len);\
  strncpy(reply->lines[reply->i], buf + offset, len);\
  reply->lines[reply->i][len - 1] = '\0';\
  reply->i++;\
} while (0)

// @see https://redis.io/topics/protocol Redis Protocol specification
int command(Conn *conn, const char *cmd, Reply *reply) {
  int i, size;
  char *buf;

  size = strlen(cmd);
  buf = (char *) malloc(sizeof(char) * (size + 3));
  strncpy(buf, cmd, size);
  strncat(buf, "\r\n", 2);
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
    buf = (char *) malloc(sizeof(char) * size);
    if (fgets(buf, size, conn->fr) == NULL) {
      free(buf);
      fprintf(stderr, "fgets(3): returns NULL when execute `%s` to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
      return MY_ERR_CODE;
    }
    switch (buf[0]) {
      case '+':
        COPY_REPLY_LINE_WITHOUT_META(reply, buf, 1);
        break;
      case '-':
        COPY_REPLY_LINE_WITHOUT_META(reply, buf, 1);
        reply->err = 1;
        break;
      case ':':
        COPY_REPLY_LINE_WITHOUT_META(reply, buf, 1);
        break;
      case '$':
        size = atoi(buf + 1);
        if (size > -1) {
          size += 3;
          ++i;
        }
        break;
      case '*':
        i += atoi(buf + 1);
        break;
      default:
        // bulk string
        COPY_REPLY_LINE_WITHOUT_META(reply, buf, 0);
        size = DEFAULT_REPLY_SIZE;
        break;
    }
    free(buf);
  }

  if (reply->err) {
    fprintf(stderr, "%s to %s:%s\n", cmd, conn->addr.host, conn->addr.port);
    fprintf(stderr, "Error: %s\n", reply->lines[0]);
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

void freeReply(Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) free(reply->lines[i]);
  free(reply->lines);
  reply->lines = NULL;
  reply->i = reply->size = reply->err = 0;
}

