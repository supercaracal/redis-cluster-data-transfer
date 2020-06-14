#ifndef GENERIC_H_
#define GENERIC_H_

#include <stdio.h>
#include <stdlib.h>

#define MY_OK_CODE 0
#define MY_ERR_CODE -1

#define ASSERT(ret) do {\
  if ((ret) == MY_ERR_CODE) exit(1);\
} while (0)

#define ASSERT_MALLOC(p, msg) do {\
  if ((p) == NULL) {\
    fprintf(stderr, "malloc(3): %s\n", (msg));\
    exit(1);\
  }\
} while (0)

#define ASSERT_REALLOC(p, msg) do {\
  if ((p) == NULL) {\
    fprintf(stderr, "realloc(3): %s\n", (msg));\
    exit(1);\
  }\
} while (0)

#define PRINT_BINARY(buf, size) do {\
  int __i;\
  for (__i = 0; __i < (size); ++__i) {\
    printf("%02x ", ((unsigned char *) (buf))[__i]);\
  }\
} while (0)

#define PRINT_MIXED_BINARY(buf, size) do {\
  int __i;\
  for (__i = 0; __i < (size); ++__i) {\
    if ((' ' <= (buf)[__i] && (buf)[__i] <= '~') || (buf)[__i] == '\r' || (buf)[__i] == '\n') {\
      printf("%c", (buf)[__i]);\
    } else {\
      printf("%02x", ((unsigned char *) (buf))[__i]);\
    }\
  }\
} while (0)

#endif  // GENERIC_H_
