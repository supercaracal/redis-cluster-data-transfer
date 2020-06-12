#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "./net.h"

#define CLUSTER_SLOT_SIZE 16384
#define ANY_NODE_OK -2

#define FIND_CONN(c, i) (c->nodes[c->slots[i]])
#define FIND_CONN2(c, i) (c.nodes[c.slots[i]])

typedef struct { int size, i; Conn **nodes; int slots[CLUSTER_SLOT_SIZE]; } Cluster;

int fetchClusterState(const char *, Cluster *);
Cluster *copyClusterState(const Cluster *);
int freeClusterState(Cluster *);
void printClusterSlots(const Cluster *);
void printClusterNodes(const Cluster *);
int key2slot(const Cluster *cluster, const char *);
int countKeysInSlot(Conn *, int);

#endif  // CLUSTER_H_
