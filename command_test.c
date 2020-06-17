#include <string.h>
#include "./generic.h"
#include "./command.h"

// basic parse
static void TestCommandParseReplyLine001(void) {
  int i, j, ret;

  struct {
    char *buf; int expRC; int expN; char *expL; int expLS; ReplyType expLT;
  } c[4] = {
    {"+OK\r\n", 0, 1, "OK", 2, STRING},
    {"-ERR you wrong\r\n", 0, 1, "ERR you wrong", 13, ERR},
    {":12345\r\n", 0, 1, "12345", 5, INTEGER},
    {"$-1\r\n", 0, 1, "", 0, NIL},
  };

  Reply r, *reply;
  reply = &r;

  for (i = 0; i < CASE_CNT(c); ++i) {
    INIT_REPLY(reply);

    ret = PublicForTestParseReplyLine(c[i].buf, reply);
    TEST_INT(i, ret == c[i].expRC, "return code", c[i].expRC, ret);
    TEST_INT(i, reply->i == c[i].expN, "number of reply lines", c[i].expN, reply->i);
    for (j = 0; j < c[i].expN; ++j) {
      TEST_INT(i, reply->sizes[j] == c[i].expLS, "reply line size", c[i].expLS, reply->sizes[j]);
      if (c[i].expLT != NIL) {
        TEST_STR(i, strncmp(reply->lines[j], c[i].expL, c[i].expLS) == 0, "reply line string", c[i].expL, reply->lines[j]);
      }
      TEST_STR(i, reply->types[j] == c[i].expLT, "reply line type", getReplyTypeCode(c[i].expLT), getReplyTypeCode(reply->types[j]));
    }

    freeReply(reply);
  }
}

// Skip previous remained reply
static void TestCommandParseReplyLine002(void) {
  int i, j, ret;
  Reply r, *reply;

  char *buf[2] = {
    "\r\n",
    "+OK\r\n",
  };

  struct {
    char *line; int size; ReplyType type;
  } c[1] = {
    {"OK", 2, STRING},
  };

  reply = &r;
  INIT_REPLY(reply);
  for (i = 1, j = 0, ret = MY_OK_CODE; i > 0; --i, ++j) {
    ret = PublicForTestParseReplyLine(buf[j], reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == CASE_CNT(c), "number of reply lines", CASE_CNT(c), reply->i);

  for (i = 0; i < CASE_CNT(c); ++i) {
    TEST_INT(i+1, reply->sizes[i] == c[i].size, "reply line size", c[i].size, reply->sizes[i]);
    TEST_STR(i+1, strncmp(reply->lines[i], c[i].line, c[i].size) == 0, "reply line string", c[i].line, reply->lines[i]);
    TEST_STR(i+1, reply->types[i] == c[i].type, "reply line type", getReplyTypeCode(c[i].type), getReplyTypeCode(reply->types[i]));
  }

  freeReply(reply);
}

// CLUSTER SLOTS: Redis version 3
static void TestCommandParseReplyLine003(void) {
  int i, j, ret;
  Reply r, *reply;

  char *buf[34] = {
    "*3\r\n",
    "*4\r\n",
    ":5461\r\n",
    ":10922\r\n",
    "*2\r\n",
    "$10\r\n",
    "172.20.0.9\r\n",
    ":6379\r\n",
    "*2\r\n",
    "$10\r\n",
    "172.20.0.7\r\n",
    ":6379\r\n",
    "*4\r\n",
    ":10923\r\n",
    ":16383\r\n",
    "*2\r\n",
    "$10\r\n",
    "172.20.0.2\r\n",
    ":6379\r\n",
    "*2\r\n",
    "$11\r\n",
    "172.20.0.12\r\n",
    ":6379\r\n",
    "*4\r\n",
    ":0\r\n",
    ":5460\r\n",
    "*2\r\n",
    "$10\r\n",
    "172.20.0.3\r\n",
    ":6379\r\n",
    "*2\r\n",
    "$11\r\n",
    "172.20.0.13\r\n",
    ":6379\r\n",
  };

  struct {
    char *line; int size; ReplyType type;
  } c[18] = {
    {"5461", 4, INTEGER},
    {"10922", 5, INTEGER},
    {"172.20.0.9", 10, STRING},
    {"6379", 4, INTEGER},
    {"172.20.0.7", 10, STRING},
    {"6379", 4, INTEGER},
    {"10923", 5, INTEGER},
    {"16383", 5, INTEGER},
    {"172.20.0.2", 10, STRING},
    {"6379", 4, INTEGER},
    {"172.20.0.12", 11, STRING},
    {"6379", 4, INTEGER},
    {"0", 1, INTEGER},
    {"5460", 4, INTEGER},
    {"172.20.0.3", 10, STRING},
    {"6379", 4, INTEGER},
    {"172.20.0.13", 11, STRING},
    {"6379", 4, INTEGER},
  };

  reply = &r;
  INIT_REPLY(reply);
  for (i = 1, j = 0, ret = MY_OK_CODE; i > 0; --i, ++j) {
    ret = PublicForTestParseReplyLine(buf[j], reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == CASE_CNT(c), "number of reply lines", CASE_CNT(c), reply->i);

  for (i = 0; i < CASE_CNT(c); ++i) {
    TEST_INT(i+1, reply->sizes[i] == c[i].size, "reply line size", c[i].size, reply->sizes[i]);
    TEST_STR(i+1, strncmp(reply->lines[i], c[i].line, c[i].size) == 0, "reply line string", c[i].line, reply->lines[i]);
    TEST_STR(i+1, reply->types[i] == c[i].type, "reply line type", getReplyTypeCode(c[i].type), getReplyTypeCode(reply->types[i]));
  }

  freeReply(reply);
}

// CLUSTER SLOTS: Redis version 5
static void TestCommandParseReplyLine004(void) {
  int i, j, ret;
  Reply r, *reply;

  char *buf[46] = {
    "*3\r\n",
    "*4\r\n",
    ":5461\r\n",
    ":10922\r\n",
    "*3\r\n",
    "$11\r\n",
    "172.20.0.10\r\n",
    ":6379\r\n",
    "$40\r\n",
    "11272f8ae3164104f179030e73ce8c10d87bbf1a\r\n",
    "*3\r\n",
    "$10\r\n",
    "172.20.0.5\r\n",
    ":6379\r\n",
    "$40\r\n",
    "98a78f4f3deefd6b3168c86069eb8047088043de\r\n",
    "*4\r\n",
    ":10923\r\n",
    ":16383\r\n",
    "*3\r\n",
    "$10\r\n",
    "172.20.0.4\r\n",
    ":6379\r\n",
    "$40\r\n",
    "6c77d7e7a269ea160bc09fb0c57ac0795bed05e4\r\n",
    "*3\r\n",
    "$10\r\n",
    "172.20.0.6\r\n",
    ":6379\r\n",
    "$40\r\n",
    "22903878f6af8e17ea114e35fd3856ce046ddf8c\r\n",
    "*4\r\n",
    ":0\r\n",
    ":5460\r\n",
    "*3\r\n",
    "$10\r\n",
    "172.20.0.8\r\n",
    ":6379\r\n",
    "$40\r\n",
    "06dd3649b8cd60d308f8cba736e201057b469573\r\n",
    "*3\r\n",
    "$11\r\n",
    "172.20.0.11\r\n",
    ":6379\r\n",
    "$40\r\n",
    "79aadb4c9439e3d349521e170d1dbe5a61682360\r\n",
  };

  struct {
    char *line; int size; ReplyType type;
  } c[24] = {
    {"5461", 4, INTEGER},
    {"10922", 5, INTEGER},
    {"172.20.0.10", 11, STRING},
    {"6379", 4, INTEGER},
    {"11272f8ae3164104f179030e73ce8c10d87bbf1a", 40, STRING},
    {"172.20.0.5", 10, STRING},
    {"6379", 4, INTEGER},
    {"98a78f4f3deefd6b3168c86069eb8047088043de", 40, STRING},
    {"10923", 5, INTEGER},
    {"16383", 5, INTEGER},
    {"172.20.0.4", 10, STRING},
    {"6379", 4, INTEGER},
    {"6c77d7e7a269ea160bc09fb0c57ac0795bed05e4", 40, STRING},
    {"172.20.0.6", 10, STRING},
    {"6379", 4, INTEGER},
    {"22903878f6af8e17ea114e35fd3856ce046ddf8c", 40, STRING},
    {"0", 1, INTEGER},
    {"5460", 4, INTEGER},
    {"172.20.0.8", 10, STRING},
    {"6379", 4, INTEGER},
    {"06dd3649b8cd60d308f8cba736e201057b469573", 40, STRING},
    {"172.20.0.11", 11, STRING},
    {"6379", 4, INTEGER},
    {"79aadb4c9439e3d349521e170d1dbe5a61682360", 40, STRING},
  };

  reply = &r;
  INIT_REPLY(reply);
  for (i = 1, j = 0, ret = MY_OK_CODE; i > 0; --i, ++j) {
    ret = PublicForTestParseReplyLine(buf[j], reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == CASE_CNT(c), "number of reply lines", CASE_CNT(c), reply->i);

  for (i = 0; i < CASE_CNT(c); ++i) {
    TEST_INT(i+1, reply->sizes[i] == c[i].size, "reply line size", c[i].size, reply->sizes[i]);
    TEST_STR(i+1, strncmp(reply->lines[i], c[i].line, c[i].size) == 0, "reply line string", c[i].line, reply->lines[i]);
    TEST_STR(i+1, reply->types[i] == c[i].type, "reply line type", getReplyTypeCode(c[i].type), getReplyTypeCode(reply->types[i]));
  }

  freeReply(reply);
}

// CLUSTER NODES
static void TestCommandParseReplyLine005(void) {
  int i, j, ret;
  Reply r, *reply;

  char *buf[7] = {
    "$705\r\n",
    "a385e7c705f79f68513e56d9634043c445e7f174 172.20.0.12:6379 slave 26a638962d7b3ea8be838d56e37158dccf875ce0 0 1592308723494 4 connected\n",
    "d7cff2a2b06bf93d8c1578fe1a3a9cbd53776447 172.20.0.9:6379 master - 0 1592308722988 2 connected 5461-10922\n",
    "26a638962d7b3ea8be838d56e37158dccf875ce0 172.20.0.2:6379 master - 0 1592308723997 3 connected 10923-16383\n",
    "7c277c08ca17c85d916da7559a96461d8791c0ae 172.20.0.3:6379 myself,master - 0 0 1 connected 0-5460\n",
    "9d1d771b9635f60a59733d3e668ba710d72ee663 172.20.0.13:6379 slave 7c277c08ca17c85d916da7559a96461d8791c0ae 0 1592308724512 5 connected\n",
    "e55c5cf9262cc12d56ab9699124efb01b9f50431 172.20.0.7:6379 slave d7cff2a2b06bf93d8c1578fe1a3a9cbd53776447 0 1592308724512 6 connected\n",
  };

  struct {
    char *line; int size; ReplyType type;
  } c[6] = {
    {"a385e7c705f79f68513e56d9634043c445e7f174 172.20.0.12:6379 slave 26a638962d7b3ea8be838d56e37158dccf875ce0 0 1592308723494 4 connected", 132, STRING},
    {"d7cff2a2b06bf93d8c1578fe1a3a9cbd53776447 172.20.0.9:6379 master - 0 1592308722988 2 connected 5461-10922", 104, STRING},
    {"26a638962d7b3ea8be838d56e37158dccf875ce0 172.20.0.2:6379 master - 0 1592308723997 3 connected 10923-16383", 105, STRING},
    {"7c277c08ca17c85d916da7559a96461d8791c0ae 172.20.0.3:6379 myself,master - 0 0 1 connected 0-5460", 95, STRING},
    {"9d1d771b9635f60a59733d3e668ba710d72ee663 172.20.0.13:6379 slave 7c277c08ca17c85d916da7559a96461d8791c0ae 0 1592308724512 5 connected", 132, STRING},
    {"e55c5cf9262cc12d56ab9699124efb01b9f50431 172.20.0.7:6379 slave d7cff2a2b06bf93d8c1578fe1a3a9cbd53776447 0 1592308724512 6 connected", 131, STRING},
  };

  reply = &r;
  INIT_REPLY(reply);
  for (i = 1, j = 0, ret = MY_OK_CODE; i > 0; --i, ++j) {
    ret = PublicForTestParseReplyLine(buf[j], reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == CASE_CNT(c), "number of reply lines", CASE_CNT(c), reply->i);

  for (i = 0; i < CASE_CNT(c); ++i) {
    TEST_INT(i+1, reply->sizes[i] == c[i].size, "reply line size", c[i].size, reply->sizes[i]);
    TEST_STR(i+1, strncmp(reply->lines[i], c[i].line, c[i].size) == 0, "reply line string", c[i].line, reply->lines[i]);
    TEST_STR(i+1, reply->types[i] == c[i].type, "reply line type", getReplyTypeCode(c[i].type), getReplyTypeCode(reply->types[i]));
  }

  freeReply(reply);
}

// Meta chars
static void TestCommandParseReplyLine006(void) {
  int i, j, ret;
  Reply r, *reply;

  char *buf[10] = {
    "$1\r\n",
    "+\r\n",
    "$1\r\n",
    "-\r\n",
    "$1\r\n",
    ":\r\n",
    "$1\r\n",
    "$\r\n",
    "$1\r\n",
    "*\r\n",
  };

  struct {
    char *line; int size; ReplyType type;
  } c[5] = {
    {"+", 1, STRING},
    {"-", 1, STRING},
    {":", 1, STRING},
    {"$", 1, STRING},
    {"*", 1, STRING},
  };

  reply = &r;
  INIT_REPLY(reply);
  for (i = 5, j = 0, ret = MY_OK_CODE; i > 0; --i, ++j) {
    ret = PublicForTestParseReplyLine(buf[j], reply);
    if (ret == MY_ERR_CODE) break;
    i += ret;
  }

  TEST_INT(0, ret == MY_OK_CODE, "return code", MY_OK_CODE, ret);
  TEST_INT(0, reply->i == CASE_CNT(c), "number of reply lines", CASE_CNT(c), reply->i);

  for (i = 0; i < CASE_CNT(c); ++i) {
    TEST_INT(i+1, reply->sizes[i] == c[i].size, "reply line size", c[i].size, reply->sizes[i]);
    TEST_STR(i+1, strncmp(reply->lines[i], c[i].line, c[i].size) == 0, "reply line string", c[i].line, reply->lines[i]);
    TEST_STR(i+1, reply->types[i] == c[i].type, "reply line type", getReplyTypeCode(c[i].type), getReplyTypeCode(reply->types[i]));
  }

  freeReply(reply);
}

void TestCommand(void) {
  TestCommandParseReplyLine001();
  TestCommandParseReplyLine002();
  TestCommandParseReplyLine003();
  TestCommandParseReplyLine004();
  TestCommandParseReplyLine005();
  TestCommandParseReplyLine006();
}
