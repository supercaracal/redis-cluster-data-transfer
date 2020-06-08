#ifndef CLUSTER_H_
#define CLUSTER_H_

int fetchClusterState(const char *, Cluster *);
Cluster *copyClusterState(const Cluster *);
int freeClusterState(Cluster *);
void printClusterSlots(const Cluster *);
void printClusterNodes(const Cluster *);
int key2slot(const Cluster *cluster, const char *);
int countKeysInSlot(Conn *, int);

#endif // CLUSTER_H_
