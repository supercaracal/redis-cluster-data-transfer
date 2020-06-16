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

  for (i = 0; i < CASE_CNT(c); ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf, c[i].size, reply);
    TEST_INT(i, ret == c[i].expRC, "return code", c[i].expRC, ret);
    TEST_INT(i, reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(i, reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_STR(i, strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string", c[i].expL, reply->lines[j]);
      TEST_STR(i, reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Simple String: fragment parse
static void TestCommandRawParseRawReply002(void) {
  int i, j, ret;

  struct {
    char *buf1; int size1; int expRC1; char *buf2; int size2; int expRC2; int expN; char *expL; int expLS; ReplyType expLT;
  } c[42] = {
    {"+O", 2, NEED_MORE_REPLY, "K\r\n", 3, MY_OK_CODE, 1, "OK", 2, STRING},
    {"-ERR ", 5, NEED_MORE_REPLY, "you wrong\r\n", 11, MY_OK_CODE, 1, "ERR you wrong", 13, ERR},
    {":123", 4, NEED_MORE_REPLY, "45\r\n", 4, MY_OK_CODE, 1, "12345", 5, INTEGER},
    {"+", 1, NEED_MORE_REPLY, "OK\r\n+OK\r\n", 9, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+O", 2, NEED_MORE_REPLY, "K\r\n+OK\r\n", 8, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK", 3, NEED_MORE_REPLY, "\r\n+OK\r\n", 7, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r", 4, NEED_MORE_REPLY, "\n+OK\r\n", 6, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r\n", 5, MY_OK_CODE, "+OK\r\n", 5, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r\n+", 6, NEED_MORE_REPLY, "OK\r\n", 4, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r\n+O", 7, NEED_MORE_REPLY, "K\r\n", 3, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r\n+OK", 8, NEED_MORE_REPLY, "\r\n", 2, MY_OK_CODE, 2, "OK", 2, STRING},
    {"+OK\r\n+OK\r", 9, NEED_MORE_REPLY, "\n", 1, MY_OK_CODE, 2, "OK", 2, STRING},
    {"-", 1, NEED_MORE_REPLY, "ERR boo\r\n-ERR boo\r\n", 19, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-E", 2, NEED_MORE_REPLY, "RR boo\r\n-ERR boo\r\n", 18, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ER", 3, NEED_MORE_REPLY, "R boo\r\n-ERR boo\r\n", 17, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR", 4, NEED_MORE_REPLY, " boo\r\n-ERR boo\r\n", 16, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR ", 5, NEED_MORE_REPLY, "boo\r\n-ERR boo\r\n", 15, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR b", 6, NEED_MORE_REPLY, "oo\r\n-ERR boo\r\n", 14, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR bo", 7, NEED_MORE_REPLY, "o\r\n-ERR boo\r\n", 13, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo", 8, NEED_MORE_REPLY, "\r\n-ERR boo\r\n", 12, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r", 9, NEED_MORE_REPLY, "\n-ERR boo\r\n", 11, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n", 10, MY_OK_CODE, "-ERR boo\r\n", 10, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-", 11, NEED_MORE_REPLY, "ERR boo\r\n", 9, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-E", 12, NEED_MORE_REPLY, "RR boo\r\n", 8, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ER", 13, NEED_MORE_REPLY, "R boo\r\n", 7, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR", 14, NEED_MORE_REPLY, " boo\r\n", 6, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR ", 15, NEED_MORE_REPLY, "boo\r\n", 5, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR b", 16, NEED_MORE_REPLY, "oo\r\n", 4, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR bo", 17, NEED_MORE_REPLY, "o\r\n", 3, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR boo", 18, NEED_MORE_REPLY, "\r\n", 2, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {"-ERR boo\r\n-ERR boo\r", 19, NEED_MORE_REPLY, "\n", 1, MY_OK_CODE, 2, "ERR boo", 7, ERR},
    {":", 1, NEED_MORE_REPLY, "123\r\n:123\r\n", 11, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":1", 2, NEED_MORE_REPLY, "23\r\n:123\r\n", 10, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":12", 3, NEED_MORE_REPLY, "3\r\n:123\r\n", 9, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123", 4, NEED_MORE_REPLY, "\r\n:123\r\n", 8, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r", 5, NEED_MORE_REPLY, "\n:123\r\n", 7, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n", 6, MY_OK_CODE, ":123\r\n", 6, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n:", 7, NEED_MORE_REPLY, "123\r\n", 5, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n:1", 8, NEED_MORE_REPLY, "23\r\n", 4, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n:12", 9, NEED_MORE_REPLY, "3\r\n", 3, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n:123", 10, NEED_MORE_REPLY, "\r\n", 2, MY_OK_CODE, 2, "123", 3, INTEGER},
    {":123\r\n:123\r", 11, NEED_MORE_REPLY, "\n", 1, MY_OK_CODE, 2, "123", 3, INTEGER},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < CASE_CNT(c); ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf1, c[i].size1, reply);
    TEST_INT(i, ret == c[i].expRC1, "return code", c[i].expRC1, ret);

    ret = PublicForTestParseRawReply(c[i].buf2, c[i].size2, reply);
    TEST_INT(i, ret == c[i].expRC2, "return code", c[i].expRC2, ret);
    TEST_INT(i, reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(i, reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_STR(i, strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string", c[i].expL, reply->lines[j]);
      TEST_STR(i, reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Bulk String: basic parse
static void TestCommandRawParseRawReply003(void) {
  int i, j, ret;

  struct {
    char *buf; int size; int expRC; int expN; char *expL; int expLS; ReplyType expLT;
  } c[3] = {
    {"$9\r\nfoobarbaz\r\n", 15, MY_OK_CODE, 1, "foobarbaz", 9, RAW},
    {"$9\r\nfoobarbaz\r\n$9\r\nfoobarbaz\r\n", 30, MY_OK_CODE, 2, "foobarbaz", 9, RAW},
    {"$-1\r\n", 5, MY_OK_CODE, 1, NULL, 0, NIL},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < CASE_CNT(c); ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf, c[i].size, reply);
    TEST_INT(i, ret == c[i].expRC, "return code", c[i].expRC, ret);
    TEST_INT(i, reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(i, reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      if (c[i].expLT == RAW) {
        TEST_RAW(i, strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string",
            c[i].expL, c[i].expLS, reply->lines[j], reply->sizes[j]);
      }
      TEST_STR(i, reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Bulk String: fragment parse
static void TestCommandRawParseRawReply004(void) {
  int i, j, ret;

  struct {
    char *buf1; int size1; int expRC1; char *buf2; int size2; int expRC2; int expN; char *expL; int expLS; ReplyType expLT;
  } c[18] = {
    {"$3\r\nfo", 6, NEED_MORE_REPLY, "o\r\n", 3, MY_OK_CODE, 1, "foo", 3, RAW},
    {"$", 1, NEED_MORE_REPLY, "3\r\nfoo\r\n$3\r\nfoo\r\n", 17, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3", 2, NEED_MORE_REPLY, "\r\nfoo\r\n$3\r\nfoo\r\n", 16, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r", 3, NEED_MORE_REPLY, "\nfoo\r\n$3\r\nfoo\r\n", 15, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\n", 4, NEED_MORE_REPLY, "foo\r\n$3\r\nfoo\r\n", 14, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nf", 5, NEED_MORE_REPLY, "oo\r\n$3\r\nfoo\r\n", 13, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfo", 6, NEED_MORE_REPLY, "o\r\n$3\r\nfoo\r\n", 12, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo", 7, NEED_MORE_REPLY, "\r\n$3\r\nfoo\r\n", 11, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r", 8, NEED_MORE_REPLY, "\n$3\r\nfoo\r\n", 10, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n", 9, MY_OK_CODE, "$3\r\nfoo\r\n", 9, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$", 10, NEED_MORE_REPLY, "3\r\nfoo\r\n", 8, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3", 11, NEED_MORE_REPLY, "\r\nfoo\r\n", 7, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r", 12, NEED_MORE_REPLY, "\nfoo\r\n", 6, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r\n", 13, NEED_MORE_REPLY, "foo\r\n", 5, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r\nf", 14, NEED_MORE_REPLY, "oo\r\n", 4, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r\nfo", 15, NEED_MORE_REPLY, "o\r\n", 3, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r\nfoo", 16, NEED_MORE_REPLY, "\r\n", 2, MY_OK_CODE, 2, "foo", 3, RAW},
    {"$3\r\nfoo\r\n$3\r\nfoo\r", 17, NEED_MORE_REPLY, "\n", 1, MY_OK_CODE, 2, "foo", 3, RAW},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < CASE_CNT(c); ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseRawReply(c[i].buf1, c[i].size1, reply);
    TEST_INT(i, ret == c[i].expRC1, "return code", c[i].expRC1, ret);

    ret = PublicForTestParseRawReply(c[i].buf2, c[i].size2, reply);
    TEST_INT(i, ret == c[i].expRC2, "return code", c[i].expRC2, ret);
    TEST_INT(i, reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(i, reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      TEST_RAW(i, strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string",
          c[i].expL, c[i].expLS, reply->lines[j], reply->sizes[j]);
      TEST_STR(i, reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Complex reply
static void TestCommandRawParseRawReply005(void) {
  int ret;
  char *buf;

  buf =
    "+OK\r\n"
    "$9\r\n"
    "foobarbaz\r\n"
    ":12345\r\n"
    "$-1\r\n";

  Reply r, *reply;
  reply = &r;

  INIT_REPLY(reply);

  ret = PublicForTestParseRawReply(buf, strlen(buf), reply);
  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == 4, "number of reply lines", 4, reply->i);

  TEST_INT(0, reply->sizes[0] == 2, "reply line size", 2, reply->sizes[0]);
  TEST_STR(0, strncmp(reply->lines[0], "OK", 2) == 0, "reply line string", "OK", reply->lines[0]);
  TEST_STR(0, reply->types[0] == STRING, "reply line type", getReplyTypeCode(STRING), getReplyTypeCode(reply->types[0]));

  TEST_INT(0, reply->sizes[1] == 9, "reply line size", 9, reply->sizes[0]);
  TEST_RAW(0, strncmp(reply->lines[1], "foobarbaz", 9) == 0, "reply line string", "foobarbaz", 9, reply->lines[1], reply->sizes[1]);
  TEST_STR(0, reply->types[1] == RAW, "reply line type", getReplyTypeCode(RAW), getReplyTypeCode(reply->types[1]));

  TEST_INT(0, reply->sizes[2] == 5, "reply line size", 5, reply->sizes[2]);
  TEST_STR(0, strncmp(reply->lines[2], "12345", 5) == 0, "reply line string", "12345", reply->lines[2]);
  TEST_STR(0, reply->types[2] == INTEGER, "reply line type", getReplyTypeCode(INTEGER), getReplyTypeCode(reply->types[2]));

  TEST_INT(0, reply->sizes[3] == 0, "reply line size", 0, reply->sizes[3]);
  TEST_STR(0, reply->types[3] == NIL, "reply line type", getReplyTypeCode(NIL), getReplyTypeCode(reply->types[3]));

  freeReply(reply);
}

// Skip remained CRLFs
static void TestCommandRawParseRawReply006(void) {
  int ret;

  Reply r, *reply;
  reply = &r;

  INIT_REPLY(reply);

  ret = PublicForTestParseRawReply("\r\n+OK\r\n", 7, reply);
  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == 1, "number of reply lines", 1, reply->i);
  TEST_INT(0, reply->sizes[0] == 2, "reply line size", 2, reply->sizes[0]);
  TEST_STR(0, strncmp(reply->lines[0], "OK", 2) == 0, "reply line string", "OK", reply->lines[0]);
  TEST_STR(0, reply->types[0] == STRING, "reply line type", getReplyTypeCode(STRING), getReplyTypeCode(reply->types[0]));

  freeReply(reply);
}

void TestCommandRaw(void) {
  TestCommandRawParseRawReply001();
  TestCommandRawParseRawReply002();
  TestCommandRawParseRawReply003();
  TestCommandRawParseRawReply004();
  TestCommandRawParseRawReply005();
  TestCommandRawParseRawReply006();
}
