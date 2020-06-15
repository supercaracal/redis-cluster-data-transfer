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
  int iii;\
  for (iii = 0; iii < (size); ++iii) {\
    printf("%02x ", ((unsigned char *) (buf))[iii]);\
  }\
} while (0)

#define PRINT_MIXED_BINARY(buf, size) do {\
  int iii;\
  for (iii = 0; iii < (size); ++iii) {\
    if ((' ' <= (buf)[iii] && (buf)[iii] <= '~') || (buf)[iii] == '\r' || (buf)[iii] == '\n') {\
      printf("%c", (buf)[iii]);\
    } else {\
      printf("%02x", ((unsigned char *) (buf))[iii]);\
    }\
  }\
} while (0)

#ifdef TEST
#define TEST_NG "\033[1m\033[31m[NG]\033[m"
#define TEST_OK "\033[1m\033[32m[OK]\033[m"

#define TEST_INT(result, desc, expected, actual) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %s: expected: %d, actual: %d\n", __func__, desc, expected, actual);\
  if (!(result)) exit(1);\
} while (0)

#define TEST_STR(result, desc, expected, actual) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %s: expected: %s, actual: %s\n", __func__, desc, expected, actual);\
  if (!(result)) exit(1);\
} while (0)

#define TEST_RAW(result, desc, expected, eSize, actual, aSize) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %s: expected: ", __func__, desc);\
  PRINT_MIXED_BINARY(expected, eSize);\
  printf(", actual: ");\
  PRINT_MIXED_BINARY(actual, aSize);\
  printf("\n");\
  if (!(result)) exit(1);\
} while (0)
#endif

#endif  // GENERIC_H_
