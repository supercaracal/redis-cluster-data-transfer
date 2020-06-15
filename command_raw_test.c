#include <string.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

static void TestCommandRawParseRawReply001(void) {
  int i, ret;

  struct {
    char *buf; int size; int expRC; int expN; char *expL; int expLS; ReplyType expLT;
  } c[3] = {
    {"+OK\r\n", 5, MY_OK_CODE, 1, "OK", 2, STRING},
    {"-ERR you wrong\r\n", 16, MY_OK_CODE, 1, "ERR you wrong", 13, ERR},
    {":12345\r\n", 8, MY_OK_CODE, 1, "12345", 5, INTEGER},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 3; ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf, c[i].size, reply);

    printf("%s", ret == c[i].expRC ? TEST_OK : TEST_NG);
    printf(" TestCommandRawParseRawReply001: return code: expected: %d, actual: %d\n", c[i].expRC, ret);

    printf("%s", reply->i == c[i].expN ? TEST_OK : TEST_NG);
    printf(" TestCommandRawParseRawReply001: number of reply lines: expected: %d, actual: %d\n", c[i].expN, reply->i);

    printf("%s", reply->sizes[0] == c[i].expLS ? TEST_OK : TEST_NG);
    printf(" TestCommandRawParseRawReply001: reply line size: expected: %d, actual: %d\n", c[i].expLS, reply->sizes[0]);

    printf("%s", strncmp(reply->lines[0], c[i].expL, c[i].expLS) == 0 ? TEST_OK : TEST_NG);
    printf(" TestCommandRawParseRawReply001: reply line string: expected: %s, actual: %s\n", c[i].expL, reply->lines[0]);

    printf("%s", reply->types[0] == c[i].expLT ? TEST_OK : TEST_NG);
    printf(" TestCommandRawParseRawReply001: reply line type: expected: %s, actual: %s\n",
        getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[0]));

    freeReply(reply);
  }
}

static void TestCommandRawParseRawReply002(void) {
}

void TestCommandRaw(void) {
  TestCommandRawParseRawReply001();
  TestCommandRawParseRawReply002();
}
