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
  while (*str == '\n' || *str == '\r' || *str == ' ' || *str == '\t') ++str;
  head = str;
  while (*str != '\0') ++str;
  if (head != str) --str;
  while (*str == '\n' || *str == '\r' || *str == ' ' || *str == '\t') --str;
  if (head != str) *(++str) = '\0';

  return head;
}

static void repl(Cluster *cluster, char *buf, int size, Reply *reply) {
  char *cmd;
  int ret;

  printClusterNodes(cluster);

  while (1) {
    printf(">> ");

    if (fgets(buf, size, stdin) == NULL) break;

    cmd = trim(buf);
    if (cmd[0] == '\0') continue;

    ret = execute(cluster, cmd, reply);
    if (ret == MY_ERR_CODE) {
      freeReply(reply);
      continue;
    }
    if (ret == EXIT_LOOP) {
      freeReply(reply);
      break;
    }

    printReplyLines(reply);
    freeReply(reply);
  }
}

static void bulk(Cluster *cluster, char *buf, int size, Reply *reply) {
  char *cmd;

  while (fgets(buf, size, stdin) != NULL) {
    cmd = trim(buf);
    if (cmd[0] == '\0') continue;

    if (execute(cluster, buf, reply) == MY_ERR_CODE) {
      freeReply(reply);
      break;
    }

    printReplyLines(reply);
    freeReply(reply);
  }
}

int main(int argc, char **argv) {
  char buf[MAX_CMD_SIZE];
  Cluster cluster;
  Reply reply;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: bin/cli host:port [--bulk]\n");
    fprintf(stderr, "  --bulk    bulk execution\n");
    exit(1);
  }

  ASSERT(fetchClusterState(argv[1], &cluster));

  if (argc == 2) {
    repl(&cluster, buf, MAX_CMD_SIZE, &reply);
  } else {
    bulk(&cluster, buf, MAX_CMD_SIZE, &reply);
  }

  ASSERT(freeClusterState(&cluster));
  exit(0);
}
