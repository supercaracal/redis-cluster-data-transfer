#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "./generic.h"

int fetchClusterState(const char *, Cluster *);
Cluster *copyClusterState(const Cluster *);
int freeClusterState(Cluster *);
void printClusterSlots(const Cluster *);
void printClusterNodes(const Cluster *);

#endif // CLUSTER_H_
