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
  int iiii;\
  for (iiii = 0; iiii < (size); ++iiii) {\
    if ((' ' <= (buf)[iiii] && (buf)[iiii] <= '~') || (buf)[iiii] == '\r' || (buf)[iiii] == '\n') {\
      printf("%c", (buf)[iiii]);\
    } else {\
      printf("%02x", ((unsigned char *) (buf))[iiii]);\
    }\
  }\
} while (0)

#ifdef TEST
#define TEST_NG "\033[1m\033[31m[NG]\033[m"
#define TEST_OK "\033[1m\033[32m[OK]\033[m"
#define CASE_CNT(c) (((int) (sizeof(c) / sizeof(c[0]))))

#define TEST_INT(no, result, desc, expected, actual) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %03d: %s: expected: %d, actual: %d\n", __func__, no + 1, desc, expected, actual);\
  if (!(result)) exit(1);\
} while (0)

#define TEST_STR(no, result, desc, expected, actual) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %03d: %s: expected: %s, actual: %s\n", __func__, no + 1, desc, expected, actual);\
  if (!(result)) exit(1);\
} while (0)

#define TEST_RAW(no, result, desc, expected, eSize, actual, aSize) do {\
  printf("%s", (result) ? TEST_OK : TEST_NG);\
  printf(" %s: %03d: %s: expected: ", __func__, no + 1, desc);\
  PRINT_MIXED_BINARY(expected, eSize);\
  printf(", actual: ");\
  PRINT_MIXED_BINARY(actual, aSize);\
  printf("\n");\
  if (!(result)) exit(1);\
} while (0)
#endif

#endif  // GENERIC_H_
