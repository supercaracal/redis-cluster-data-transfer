#include <string.h>
#include "./cluster.h"
#include "./net.h"

#define DEFAULT_NODE_SIZE 8

int buildClusterState(const Reply *reply, Cluster *cluster) {
  int i, j, first, last;

  cluster->size = DEFAULT_NODE_SIZE;
  cluster->i = 0;
  cluster->nodes = (Conn **) malloc(sizeof(Conn *) * cluster->size);

  for (i = 0; i < reply->i; ++i) {
    if (i + 3 >= reply->i) break;
    if (reply->types[i] != INTEGER || reply->types[i+1] != INTEGER || reply->types[i+2] != STRING) continue;

    if (cluster->i == cluster->size) {
      cluster->size *= 2;
      cluster->nodes = (Conn **) realloc(cluster->nodes, sizeof(Conn *) * cluster->size);
    }

    cluster->nodes[cluster->i] = (Conn *) malloc(sizeof(Conn));

    first = atoi(reply->lines[i]);
    last = atoi(reply->lines[i+1]);
    strcpy(cluster->nodes[cluster->i]->addr.host, reply->lines[i+2]);
    strcpy(cluster->nodes[cluster->i]->addr.port, reply->lines[i+3]);

    for (j = first; j <= last; ++j) cluster->slots[j] = cluster->i;
    cluster->i++;
  }

  if (cluster->i == 0) {
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

int freeClusterState(Cluster *c) {
  int i;

  for (i = 0; i < c->i; ++i) {
    if (freeConnection(c->nodes[i]) == MY_ERR_CODE) return MY_ERR_CODE;
    free(c->nodes[i]);
  }
  free(c->nodes);
  c->nodes = NULL;
  c->size = c->i = 0;
  return MY_OK_CODE;
}

void printClusterSlots(const Cluster *c) {
  int i;
  Conn *conn;

  for (i = 0; i < CLUSTER_SLOT_SIZE; ++i) {
    conn = FIND_CONN(c, i);
    fprintf(stdout, "%05d\t%s:%s\n", i, conn->addr.host, conn->addr.port);
  }
}
