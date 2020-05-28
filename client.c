#include <string.h>
#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#define MAX_KEY_SIZE 256
#define ANY_NODE_OK -2
#define EXIT_LOOP -3

static int isKeylessCommand(const char *cmd) {
  while (*cmd != '\0') {
    if (*cmd == ' ') return 0;
    ++cmd;
  }
  return 1;
}

static int key2slot(Cluster *cluster, const char *cmd) {
  char cBuf[MAX_CMD_SIZE], kBuf[MAX_KEY_SIZE], *line;
  int slot, ret;
  Reply reply;

  if (isKeylessCommand(cmd)) return ANY_NODE_OK;

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
  Cluster cluster;
  Reply reply;
  int ret;

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
