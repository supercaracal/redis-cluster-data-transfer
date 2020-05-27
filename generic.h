#ifndef GENERIC_H_
#define GENERIC_H_

#include <stdio.h>
#include <stdlib.h>

#define MY_OK_CODE 0
#define MY_ERR_CODE -1
#define MAX_HOST_SIZE 256
#define MAX_PORT_SIZE 8
#define MAX_CMD_SIZE 4096
#define CLUSTER_SLOT_SIZE 16384

#define ASSERT(ret) do {\
  if (ret == MY_ERR_CODE) exit(1);\
} while (0)

#define ASSERT_MALLOC(p, msg) do {\
  if (p == NULL) {\
    fprintf(stderr, "malloc(3): %s\n", msg);\
    exit(1);\
  }\
} while (0)

#define ASSERT_REALLOC(p, msg) do {\
  if (p == NULL) {\
    fprintf(stderr, "realloc(3): %s\n", msg);\
    exit(1);\
  }\
} while (0)

#define LAST_LINE(r) (r->i > 0 ? r->lines[r->i - 1] : NULL)
#define LAST_LINE2(r) (r.i > 0 ? r.lines[r.i - 1] : NULL)
#define FIND_CONN(c, i) (c->nodes[c->slots[i]])

typedef struct { char host[MAX_HOST_SIZE], port[MAX_PORT_SIZE]; } HostPort;
typedef struct { FILE *fw, *fr; HostPort addr; } Conn;
typedef struct { int size, i; Conn **nodes; int slots[CLUSTER_SLOT_SIZE]; } Cluster;
typedef enum { STRING, INTEGER } ReplyType;
typedef struct { int size, i, err; char **lines; ReplyType *types; } Reply;

#endif // GENERIC_H_
