#ifndef COPY_H_
#define COPY_H_

#include "./cluster.h"

typedef struct { int copied, skipped, failed, found; } MigrationResult;

int copyKeys(const Cluster *, const Cluster *, int, int, MigrationResult *);

#endif  // COPY_H_
