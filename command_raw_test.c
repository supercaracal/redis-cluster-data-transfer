#include <string.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

static void TestCommandRawParseRawReply001(void) {
  int i;

  struct {
    char *buf; int size; int expN; char *expL; int expLS;
  } c[3] = {
    {"+OK\r\n", 5, 1, "OK", 2},
    {"-ERR you wrong\r\n", 16, 1, "ERR you wrong", 13},
    {":12345\r\n", 8, 1, "12345", 5},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 3; ++i) {
    INIT_REPLY(reply);

    PublicForTestParseRawReply(c[i].buf, c[i].size, reply);

    reply->i == c[i].expN ? printf(TEST_OK) : printf(TEST_NG);
    printf(" TestCommandRawParseRawReply001: number of reply lines: expected: %d, actual: %d\n", c[i].expN, reply->i);

    strncmp(reply->lines[0], c[i].expL, c[i].expLS) == 0 ? printf(TEST_OK) : printf(TEST_NG);
    printf(" TestCommandRawParseRawReply001: reply line string: expected: %s, actual: %s\n", c[i].expL, reply->lines[0]);

    freeReply(reply);
  }
}

void TestCommandRaw(void) {
  TestCommandRawParseRawReply001();
}
