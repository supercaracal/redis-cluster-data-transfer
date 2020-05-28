#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#define MAX_KEY_SIZE 256
#define ANY_NODE_OK -2

static int key2slot(Cluster *cluster, const char *cmd) {
  char cBuf[MAX_CMD_SIZE], kBuf[MAX_KEY_SIZE], *line;
  int slot, ret;
  Reply reply;

  snprintf(cBuf, MAX_CMD_SIZE, "COMMAND GETKEYS %s", cmd);
  ret = command(cluster->nodes[0], cBuf, &reply);
  if (ret == MY_ERR_CODE) {
    freeReply(&reply);
    return ret;
  }

  line = LAST_LINE2(reply);
  if (line == NULL || strlen(line) == 0) {
    freeReply(&reply);
    return ANY_NODE_OK; // FIXME: blank is a bug
  }

  strcpy(kBuf, line);
  freeReply(&reply);

  snprintf(cBuf, MAX_CMD_SIZE, "CLUSTER KEYSLOT %s", kBuf);
  ret = command(cluster->nodes[0], cBuf, &reply);
  if (ret == MY_ERR_CODE) {
    freeReply(&reply);
    return ret;
  }

  line = LAST_LINE2(reply);
  if (line == NULL) {
    fprintf(stderr, "Could not get slot number of key: %s\n", kBuf);
    freeReply(&reply);
    return MY_ERR_CODE;
  }

  slot = atoi(line);
  freeReply(&reply);

  return slot;
}

static int execute(Cluster *cluster, const char *cmd, Reply *reply) {
  int ret;
  Conn *conn;

  ret = key2slot(cluster, cmd);
  if (ret == MY_ERR_CODE) return ret;

  conn = ret == ANY_NODE_OK ? cluster->nodes[0] : FIND_CONN(cluster, ret);

  ret = command(conn, cmd, reply);
  if (ret == MY_ERR_CODE) return ret;

  return MY_OK_CODE;
}

static inline void trim(char *str) {
  if (str == NULL) return;
  while (*str != '\0') ++str;
  --str;
  while (*str == '\n' || *str == '\r') --str;
  *(++str) = '\0';
}

int main(int argc, char **argv) {
  char buf[MAX_CMD_SIZE];
  Cluster cluster;
  Reply reply;

  if (argc != 2) {
    fprintf(stderr, "Usage: bin/cli host:port\n");
    exit(1);
  }

  ASSERT(fetchClusterState(argv[1], &cluster));
  printClusterNodes(&cluster);

  while (1) {
    printf("cli > ");

    if (fgets(buf, sizeof(buf), stdin) == NULL) {
      fprintf(stderr, "fgets(3)\n");
      continue;
    }

    trim(buf);
    if (execute(&cluster, buf, &reply) == MY_ERR_CODE) {
      freeReply(&reply);
      continue;
    }

    printReplyLines(&reply);
    freeReply(&reply);
  }

  ASSERT(freeClusterState(&cluster));
  exit(0);
}
