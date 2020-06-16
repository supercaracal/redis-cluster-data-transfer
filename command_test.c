#include <string.h>
#include "./generic.h"
#include "./command.h"

// CLUSTER SLOTS: Redis 3
static void TestCommandParseReplyLine001(void) {
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

void TestCommand(void) {
  TestCommandParseReplyLine001();
}
