#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

int main(int argc, char **argv) {
  Cluster cluster;
  Reply reply;

  if (argc != 2) {
    fprintf(stderr, "Usage: bin/getter host:port\n");
    exit(1);
  }

  ASSERT(fetchClusterState(argv[1], &cluster));

  while (1) {
    command(cluster.nodes[2], "GET key1", &reply);
    printf("\033[2K\033[1G%s", LAST_LINE2(reply));
    fflush(stdout);
    freeReply(&reply);
  }
}
