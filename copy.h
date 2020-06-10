#ifndef COPY_H_
#define COPY_H_

#include "./net.h"

typedef struct { int copied, skipped, failed, found; } MigrationResult;

int copyKeys(Conn *, Conn *, int, MigrationResult *, int);

#endif  // COPY_H_
