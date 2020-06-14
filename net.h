#ifndef NET_H_
#define NET_H_

#include <stdio.h>

#define MAX_HOST_SIZE 256
#define MAX_PORT_SIZE 8

typedef struct { char host[MAX_HOST_SIZE], port[MAX_PORT_SIZE]; } HostPort;
typedef struct { FILE *fw, *fr; HostPort addr; } Conn;

int createConnection(Conn *);
int createConnectionFromStr(const char *, Conn *);
int reconnect(Conn *);
int freeConnection(Conn *);

#endif  // NET_H_
