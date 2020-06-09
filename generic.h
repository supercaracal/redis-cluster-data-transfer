#ifndef GENERIC_H_
#define GENERIC_H_

#include <stdio.h>
#include <stdlib.h>

#define MY_OK_CODE 0
#define MY_ERR_CODE -1

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

#endif  // GENERIC_H_
