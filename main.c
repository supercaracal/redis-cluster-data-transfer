#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

int main(int argc, char **argv) {
  Conn conn;
  Reply reply;
  int slots[CLUSTER_SLOT_SIZE];

  if (argc < 3 || argc > 3) {
    fprintf(stderr, "Usage: ./main [src-host:port] [dest-host:port]\n");
    exit(1);
  }

  if (createConnection(argv[1], &conn) == MY_ERR_CODE) exit(1);
  if (command(&conn, "CLUSTER SLOTS", &reply) == MY_ERR_CODE) exit(1);
  buildSlotNodeTable(&reply, slots);

  freeReply(&reply);
  freeConnection(&conn);
  exit(0);
}
