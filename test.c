#include "./generic.h"
#include "./command_test.h"
#include "./command_raw_test.h"

int main(void) {
  TestCommand();
  TestCommandRaw();
  printf(TEST_OK " All test cases are passed.\n");
}
