#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#define EXIT_LOOP -3

static int execute(Cluster *cluster, const char *cmd, Reply *reply) {
  int ret;
  Conn *conn;

  if (strcmp(cmd, "quit") == 0) return EXIT_LOOP;
  if (strcmp(cmd, "exit") == 0) return EXIT_LOOP;

  ret = key2slot(cluster, cmd);
  if (ret == MY_ERR_CODE) return ret;

  conn = ret == ANY_NODE_OK ? cluster->nodes[0] : FIND_CONN(cluster, ret);

  ret = command(conn, cmd, reply);
  if (ret == MY_ERR_CODE) return ret;

  return MY_OK_CODE;
}

static char *trim(char *str) {
  char *head;

  if (str == NULL) return str;
  while (*str == ' ') ++str;
  head = str;
  while (*str != '\0') ++str;
  --str;
  while (*str == '\n' || *str == '\r' || *str == ' ') --str;
  *(++str) = '\0';

  return head;
}

int main(int argc, char **argv) {
  char buf[MAX_CMD_SIZE], *cmd;
  int ret;
  Cluster cluster;
  Reply reply;

  if (argc != 2) {
    fprintf(stderr, "Usage: bin/cli host:port\n");
    exit(1);
  }

  ASSERT(fetchClusterState(argv[1], &cluster));
  printClusterNodes(&cluster);

  while (1) {
    printf(">> ");

    if (fgets(buf, sizeof(buf), stdin) == NULL) {
      fprintf(stderr, "fgets(3)\n");
      continue;
    }

    cmd = trim(buf);
    ret = execute(&cluster, cmd, &reply);
    if (ret == MY_ERR_CODE) {
      freeReply(&reply);
      continue;
    }
    if (ret == EXIT_LOOP) {
      freeReply(&reply);
      break;
    }

    printReplyLines(&reply);
    freeReply(&reply);
  }

  ASSERT(freeClusterState(&cluster));
  exit(0);
}
