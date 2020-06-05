#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

int main(int argc, char **argv) {
  Cluster cluster;
  Reply reply;
  char *cmd, *line;
  int slot;

  if (argc != 2) {
    fprintf(stderr, "Usage: bin/setter host:port\n");
    exit(1);
  }

  ASSERT(fetchClusterState(argv[1], &cluster));

  cmd = "SET key1 1";
  slot = key2slot(&cluster, cmd);
  ASSERT(slot);
  ASSERT(command(FIND_CONN2(cluster, slot), cmd, &reply));
  freeReply(&reply);

  while (1) {
    command(FIND_CONN2(cluster, slot), "INCR key1", &reply);
    line = LAST_LINE2(reply);
    printf("\033[2K\033[1G%09d", atoi(line == NULL ? "0" : line));
    fflush(stdout);
    freeReply(&reply);
  }
}
