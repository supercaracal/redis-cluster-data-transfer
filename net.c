#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "./net.h"

#define MAX_TIMEOUT_SEC 5

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

static int connectServer(const HostPort *addr) {
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

int createConnection(Conn *c) {
  int sock;

  sock = connectServer(&c->addr);
  if (sock == MY_ERR_CODE) {
    fprintf(stderr, "Could not connect server: %s:%s\n", c->addr.host, c->addr.port);
    return MY_ERR_CODE;
  }

  if (setSocketOptions(sock) == MY_ERR_CODE) {
    fprintf(stderr, "Could not set socket options: %s:%s\n", c->addr.host, c->addr.port);
    return MY_ERR_CODE;
  }

  c->fw = fdopen(sock, "w");
  if (c->fw == NULL) {
    fprintf(stderr, "fdopen(3): for writing to %s:%s\n", c->addr.host, c->addr.port);
    return MY_ERR_CODE;
  }

  c->fr = fdopen(sock, "r");
  if (c->fr == NULL) {
    fprintf(stderr, "fdopen(3): for reading to %s:%s\n", c->addr.host, c->addr.port);
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

int createConnectionFromStr(const char *str, Conn *c) {
  if (parseAddress(str, &c->addr) == MY_ERR_CODE) {
    fprintf(stderr, "Could not parse argument: %s\n", str);
    return MY_ERR_CODE;
  }

  return createConnection(c);
}

int reconnect(Conn *c) {
  freeConnection(c);
  return createConnection(c);
}

int freeConnection(Conn *c) {
  fflush(c->fw);
  fflush(c->fr);

  if (fclose(c->fw) == EOF) {
    // perror("fclose(3): for write");
    // return MY_ERR_CODE;
  };
  c->fw = NULL;

  // FIXME: The socket is shared between write and read. So it is already bad descriptor.
  if (fclose(c->fr) == EOF) {
    // perror("fclose(3): for read");
    // return MY_ERR_CODE;
  }
  c->fr = NULL;

  return MY_OK_CODE;
}
