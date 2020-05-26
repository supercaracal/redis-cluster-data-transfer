#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

int main(int argc, char **argv) {
  Conn conn;
  Reply reply;
  int ret;

  if (argc < 3 || argc > 3) {
    fprintf(stderr, "Usage: ./main [src-host:port] [dest-host:port]\n");
    exit(1);
  }

  ret = createConnection(argv[1], &conn);
  ASSERT(ret);

  ret = command(&conn, "CLUSTER SLOTS", &reply);
  ASSERT(ret);
  printReplyLines(&reply);
  freeReply(&reply);

  ret = command(&conn, "CLUSTER NODES", &reply);
  ASSERT(ret);
  printReplyLines(&reply);
  freeReply(&reply);

  ret = command(&conn, "INFO", &reply);
  ASSERT(ret);
  printReplyLines(&reply);
  freeReply(&reply);

  ret = command(&conn, "GET key1", &reply);
  ASSERT(ret);
  printReplyLines(&reply);
  freeReply(&reply);

  freeConnection(&conn);
  exit(0);
}
