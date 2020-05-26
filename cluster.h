#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "./generic.h"

int buildClusterState(Reply *, Cluster *);
int freeClusterState(Cluster *);
void printClusterSlots(Cluster *);

#endif // CLUSTER_H_
