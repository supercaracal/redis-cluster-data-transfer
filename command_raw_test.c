#include <string.h>
#include "./generic.h"
#include "./command.h"
#include "./command_raw.h"

// Simple String: basic parse
static void TestCommandRawParseRawReply001(void) {
  int i, j, ret;

  struct {
    char *buf; int size; int expRC; int expN; char *expL; int expLS; ReplyType expLT;
  } c[6] = {
    {"+OK\r\n", 5, MY_OK_CODE, 1, "OK", 2, STRING},
    {"-ERR you wrong\r\n", 16, MY_OK_CODE, 1, "ERR you wrong", 13, ERR},
    {":12345\r\n", 8, MY_OK_CODE, 1, "12345", 5, INTEGER},
    {"+OK\r\n+OK\r\n", 10, MY_OK_CODE, 2, "OK", 2, STRING},
    {"-ERR you wrong\r\n-ERR you wrong\r\n", 32, MY_OK_CODE, 2, "ERR you wrong", 13, ERR},
    {":12345\r\n:12345\r\n", 16, MY_OK_CODE, 2, "12345", 5, INTEGER},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 6; ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf, c[i].size, reply);
    TEST_INT(ret == c[i].expRC, "return code", c[i].expRC, ret);
    TEST_INT(reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_STR(strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string", c[i].expL, reply->lines[j]);
      TEST_STR(reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Simple String: fragment parse
static void TestCommandRawParseRawReply002(void) {
  int i, j, ret;

  struct {
    char *buf1; int size1; int expRC1; char *buf2; int size2; int expRC2; int expN; char *expL; int expLS; ReplyType expLT;
  } c[3] = {
    {"+O", 2, NEED_MORE_REPLY, "K\r\n", 3, MY_OK_CODE, 1, "OK", 2, STRING},
    {"-ERR ", 5, NEED_MORE_REPLY, "you wrong\r\n", 11, MY_OK_CODE, 1, "ERR you wrong", 13, ERR},
    {":123", 4, NEED_MORE_REPLY, "45\r\n", 4, MY_OK_CODE, 1, "12345", 5, INTEGER},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 3; ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf1, c[i].size1, reply);
    TEST_INT(ret == c[i].expRC1, "return code", c[i].expRC1, ret);

    ret = PublicForTestParseRawReply(c[i].buf2, c[i].size2, reply);
    TEST_INT(ret == c[i].expRC2, "return code", c[i].expRC2, ret);
    TEST_INT(reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_STR(strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string", c[i].expL, reply->lines[j]);
      TEST_STR(reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Bulk String: basic parse
static void TestCommandRawParseRawReply003(void) {
  int i, j, ret;

  struct {
    char *buf; int size; int expRC; int expN; char *expL; int expLS; ReplyType expLT;
  } c[2] = {
    {"$9\r\nfoobarbaz\r\n", 15, MY_OK_CODE, 1, "foobarbaz", 9, RAW},
    {"$9\r\nfoobarbaz\r\n$9\r\nfoobarbaz\r\n", 30, MY_OK_CODE, 2, "foobarbaz", 9, RAW},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 2; ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf, c[i].size, reply);
    TEST_INT(ret == c[i].expRC, "return code", c[i].expRC, ret);
    TEST_INT(reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_RAW(strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string",
          c[i].expL, c[i].expLS, reply->lines[j], reply->sizes[j]);
      TEST_STR(reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Bulk String: fragment parse
static void TestCommandRawParseRawReply004(void) {
  int i, j, ret;

  struct {
    char *buf1; int size1; int expRC1; char *buf2; int size2; int expRC2; int expN; char *expL; int expLS; ReplyType expLT;
  } c[2] = {
    {"$3\r\nfo", 6, NEED_MORE_REPLY, "o\r\n", 3, MY_OK_CODE, 1, "foo", 3, RAW},
    {"$", 1, NEED_MORE_REPLY, "3\r\nfoo\r\n", 8, MY_OK_CODE, 1, "foo", 3, RAW},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < 2; ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf1, c[i].size1, reply);
    TEST_INT(ret == c[i].expRC1, "return code", c[i].expRC1, ret);

    ret = PublicForTestParseRawReply(c[i].buf2, c[i].size2, reply);
    TEST_INT(ret == c[i].expRC2, "return code", c[i].expRC2, ret);
    TEST_INT(reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_RAW(strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string",
          c[i].expL, c[i].expLS, reply->lines[j], reply->sizes[j]);
      TEST_STR(reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

void TestCommandRaw(void) {
  TestCommandRawParseRawReply001();
  TestCommandRawParseRawReply002();
  TestCommandRawParseRawReply003();
  TestCommandRawParseRawReply004();
}
