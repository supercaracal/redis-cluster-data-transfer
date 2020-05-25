#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MY_OK_CODE 0
#define MY_ERR_CODE -1
#define MAX_HOST_SIZE 256
#define MAX_PORT_SIZE 8
#define MAX_TIMEOUT_SEC 5
#define CLUSTER_SLOT_SIZE 16384
#define DEFAULT_REPLY_LINES 16
#define DEFAULT_REPLY_SIZE 256

#define FAIL_ARGS do {\
  fprintf(stderr, "Usage: ./main [src-host:port] [dest-host:port]\n");\
  exit(1);\
} while(0)

#define FAIL_PARSE_ADDR(str) do {\
  fprintf(stderr, "Failed to parse: %s\n", str);\
  exit(1);\
} while(0)

#define FAIL_CONN(addr) do {\
  fprintf(stderr, "Could not connect server: %s:%s\n", addr.host, addr.port);\
  exit(1);\
} while(0)

#define FAIL_SOCK_OPTS(addr) do {\
  fprintf(stderr, "Could not set socket options: %s:%s\n", addr.host, addr.port);\
  exit(1);\
} while(0)

#define FAIL_CMD_EXEC(cmd) do {\
  fprintf(stderr, "Could not execute command: %s\n", cmd);\
  exit(1);\
} while(0)

#define FAIL_CMD_ERROR(cmd, reply) do {\
  fprintf(stderr, "Could not execute command: %s\n", cmd);\
  fprintf(stderr, "Error: %s\n", reply.lines[0]);\
  freeReply(&reply);\
  exit(1);\
} while(0)

#define INIT_REPLY(r) do {\
  r->size = DEFAULT_REPLY_LINES;\
  r->i = 0;\
  r->err = 0;\
  r->lines = (char **) malloc(sizeof(char *) * r->size);\
} while(0)

#define EXPAND_REPLY_IF_NEEDED(r) do {\
  if (r->i == r->size) {\
    r->size *= 2;\
    r->lines = (char **) realloc(r->lines, sizeof(char *) * r->size);\
  }\
} while(0)

#define COPY_REPLY_LINE_WITHOUT_META(reply, buf, offset) do {\
  int len = strlen(buf) + 1 - 2 - offset;\
  reply->lines[reply->i] = (char *) malloc(sizeof(char) * len);\
  strncpy(reply->lines[reply->i], buf + offset, len);\
  reply->lines[reply->i][len - 1] = '\0';\
  reply->i++;\
} while(0)

typedef struct { char host[MAX_HOST_SIZE], port[MAX_PORT_SIZE]; } HostPort;
typedef struct { HostPort addr; int sock, slots[CLUSTER_SLOT_SIZE]; } Context;
typedef struct { int size, i, err; char **lines; } Reply;

static int parseAddress(const char *str, HostPort *addr) {
  int len, i, j;

  if (str == NULL) return MY_ERR_CODE;
  len = strlen(str);
  for (i = len - 1; i >= 0; --i) {
    if (str[i] != ':') continue;
    for (j = 0; j < i; ++j) addr->host[j] = str[j];
    addr->host[j] = '\0';
    for (j = 0; i + j + 1 < len; ++j) addr->port[j] = str[i + j + 1];
    addr->port[j] = '\0';
    if (addr->host[0] == '\0' || addr->port[0] == '\0') return MY_ERR_CODE;
    return MY_OK_CODE;
  }

  return MY_ERR_CODE;
}

static int connectServer(HostPort *addr) {
  int sock, err;
  struct addrinfo hints, *res, *ai;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  err = getaddrinfo(addr->host, addr->port, &hints, &res);
  if (err != 0) {
    fprintf(stderr, "getaddrinfo(3): %s\n", gai_strerror(err));
    return MY_ERR_CODE;
  }

  for (ai = res; ai; ai = ai->ai_next) {
    sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) continue;
    if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(sock);
      continue;
    }
    freeaddrinfo(res);
    return sock;
  }

  freeaddrinfo(res);
  return MY_ERR_CODE;
}

static int setSocketOptions(int sock) {
  int on = 1;
  struct timeval tv = { MAX_TIMEOUT_SEC, 0 };

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) return MY_ERR_CODE;
  if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) == -1) return MY_ERR_CODE;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) return MY_ERR_CODE;
  if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) return MY_ERR_CODE;

  return MY_OK_CODE;
}

// @see https://redis.io/topics/protocol Redis Protocol specification
static int command(int sock, const char *cmd, int wSize, Reply *reply) {
  int len, i, rSize;
  char *buf;
  FILE *f;

  len = send(sock, cmd, wSize, 0);
  if (len < 0) {
    fprintf(stderr, "%s\n", cmd);
    perror("send(2)");
    return MY_ERR_CODE;
  }

  f = fdopen(sock, "r");
  if (f == NULL) {
    perror("fdopen(3)");
    return MY_ERR_CODE;
  }

  INIT_REPLY(reply);
  for (i = 1, rSize = DEFAULT_REPLY_SIZE; i > 0; --i) {
    EXPAND_REPLY_IF_NEEDED(reply);
    buf = (char *) malloc(sizeof(char) * rSize);
    if (fgets(buf, rSize, f) == NULL) {
      free(buf);
      break;
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
        rSize = atoi(buf + 1) + 3;
        if (rSize > -1) ++i;
        break;
      case '*':
        i += atoi(buf + 1);
        break;
      default:
        // bulk string
        COPY_REPLY_LINE_WITHOUT_META(reply, buf, 0);
        rSize = DEFAULT_REPLY_SIZE;
        break;
    }
    free(buf);
  }
  fclose(f);

  return MY_OK_CODE;
}

static void freeReply(Reply *reply) {
  int i;

  for (i = 0; i < reply->i; ++i) free(reply->lines[i]);
  free(reply->lines);
  reply->lines = NULL;
  reply->i = reply->size = reply->err = 0;
}

static void buildSlotNodeTable(Reply *reply, int *slots) {
  int i;

  fprintf(stdout, "------------------------------------------------------------\n");
  for (i = 0; i < reply->i; ++i) fprintf(stdout, "%s\n", reply->lines[i]);
}

int main(int argc, char **argv) {
  Context src, dest;
  Reply reply;

  if (argc < 3 || argc > 3) FAIL_ARGS;

  if (parseAddress(argv[1], &src.addr) == MY_ERR_CODE) FAIL_PARSE_ADDR(argv[1]);
  if (parseAddress(argv[2], &dest.addr) == MY_ERR_CODE) FAIL_PARSE_ADDR(argv[2]);

  src.sock = connectServer(&src.addr);
  if (src.sock == MY_ERR_CODE) FAIL_CONN(src.addr);
  if (setSocketOptions(src.sock) == MY_ERR_CODE) FAIL_SOCK_OPTS(src.addr);

  dest.sock = connectServer(&dest.addr);
  if (dest.sock == MY_ERR_CODE) FAIL_CONN(dest.addr);
  if (setSocketOptions(dest.sock) == MY_ERR_CODE) FAIL_SOCK_OPTS(dest.addr);

  if (command(src.sock, "CLUSTER SLOTS\r\n", 17, &reply) == MY_ERR_CODE) FAIL_CMD_EXEC("CLUSTER SLOTS");
  if (reply.err) FAIL_CMD_ERROR("CLUSTER SLOTS", reply);
  buildSlotNodeTable(&reply, src.slots);
  freeReply(&reply);

  if (command(dest.sock, "CLUSTER SLOTS\r\n", 17, &reply) == MY_ERR_CODE) FAIL_CMD_EXEC("CLUSTER SLOTS");
  if (reply.err) FAIL_CMD_ERROR("CLUSTER SLOTS", reply);
  buildSlotNodeTable(&reply, dest.slots);
  freeReply(&reply);

  close(src.sock);
  close(dest.sock);
  exit(0);
}
