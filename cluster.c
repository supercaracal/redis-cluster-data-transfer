#include <string.h>
#include "./cluster.h"
#include "./net.h"
#include "./command.h"

#define DEFAULT_NODE_SIZE 8

static int buildClusterState(const Reply *reply, Cluster *cluster) {
  int i, j, first, last;
  void *tmp;

  cluster->size = DEFAULT_NODE_SIZE;
  cluster->i = 0;
  cluster->nodes = (Conn **) malloc(sizeof(Conn *) * cluster->size);
  ASSERT_MALLOC(cluster->nodes, "for init cluster connections");

  for (i = 0; i < reply->i; ++i) {
    if (i + 3 >= reply->i) break;
    if (reply->types[i] != INTEGER || reply->types[i+1] != INTEGER || reply->types[i+2] != STRING) continue;

    if (cluster->i == cluster->size) {
      cluster->size *= 2;
      tmp = realloc(cluster->nodes, sizeof(Conn *) * cluster->size);
      ASSERT_REALLOC(tmp, "for cluster connections");
      cluster->nodes = (Conn **) tmp;
    }

    cluster->nodes[cluster->i] = (Conn *) malloc(sizeof(Conn));
    ASSERT_MALLOC(cluster->nodes[cluster->i], "for new cluster connection");

    first = atoi(reply->lines[i]);
    last = atoi(reply->lines[i+1]);
    strcpy(cluster->nodes[cluster->i]->addr.host, reply->lines[i+2]);
    strcpy(cluster->nodes[cluster->i]->addr.port, reply->lines[i+3]);

    for (j = first; j <= last; ++j) cluster->slots[j] = cluster->i;
    cluster->i++;
  }

  if (cluster->i == 0) {
    return MY_ERR_CODE;
  }

  return MY_OK_CODE;
}

int fetchClusterState(const char *str, Cluster *cluster) {
  Conn conn;
  Reply reply;
  int ret, i;

  ret = createConnectionFromStr(str, &conn);
  if (ret == MY_ERR_CODE) return ret;

  ret = command(&conn, "CLUSTER SLOTS", &reply);
  if (ret == MY_ERR_CODE) return ret;

  ret = buildClusterState(&reply, cluster);
  if (ret == MY_ERR_CODE) return ret;

  for (i = 0; i < cluster->i; ++i) {
    ret = createConnection(cluster->nodes[i]);
    if (ret == MY_ERR_CODE) return ret;
  }

  freeReply(&reply);

  ret = freeConnection(&conn);
  if (ret == MY_ERR_CODE) return ret;

  return MY_OK_CODE;
}

Cluster *copyClusterState(const Cluster *src) {
  int i;
  Cluster *dest;

  dest = (Cluster *) malloc(sizeof(Cluster));
  ASSERT_MALLOC(dest, "for init cluster on copy");

  for (i = 0; i < CLUSTER_SLOT_SIZE; ++i) dest->slots[i] = src->slots[i];
  dest->size = src->size;
  dest->i = src->i;

  dest->nodes = (Conn **) malloc(sizeof(Conn *) * dest->size);
  ASSERT_MALLOC(dest->nodes, "for init cluster connections on copy");

  for (i = 0; i < src->i; ++i) {
    dest->nodes[i] = (Conn *) malloc(sizeof(Conn));
    ASSERT_MALLOC(dest->nodes[i], "for new cluster connection on copy");

    strcpy(dest->nodes[i]->addr.host, src->nodes[i]->addr.host);
    strcpy(dest->nodes[i]->addr.port, src->nodes[i]->addr.port);

    if (createConnection(dest->nodes[i]) == MY_ERR_CODE) return NULL;
  }

  return dest;
}

int freeClusterState(Cluster *c) {
  int i;

  for (i = 0; i < c->i; ++i) {
    if (freeConnection(c->nodes[i]) == MY_ERR_CODE) return MY_ERR_CODE;
    free(c->nodes[i]);
  }
  free(c->nodes);
  c->nodes = NULL;
  c->size = c->i = 0;
  return MY_OK_CODE;
}

void printClusterSlots(const Cluster *c) {
  int i;
  Conn *conn;

  for (i = 0; i < CLUSTER_SLOT_SIZE; ++i) {
    conn = FIND_CONN(c, i);
    fprintf(stdout, "%05d\t%s:%s\n", i, conn->addr.host, conn->addr.port);
  }
}

void printClusterNodes(const Cluster *c) {
  int i;

  for (i = 0; i < c->i; ++i) {
    printf("%s:%s\n", c->nodes[i]->addr.host, c->nodes[i]->addr.port);
  }
}

int key2slot(const Cluster *cluster, const char *cmd) {
  char cBuf[MAX_CMD_SIZE], kBuf[MAX_KEY_SIZE], *line;
  int slot, ret;
  Reply reply;

  if (isKeylessCommand(cmd)) return ANY_NODE_OK;

  snprintf(cBuf, MAX_CMD_SIZE, "COMMAND GETKEYS %s", cmd);
  ret = command(cluster->nodes[0], cBuf, &reply);
  if (ret == MY_ERR_CODE) {
    line = LAST_LINE2(reply) == NULL ? "" : LAST_LINE2(reply);
    ret = strncmp(line, "ERR The command has no key arguments", 36);
    freeReply(&reply);
    return ret == 0 ? ANY_NODE_OK : MY_ERR_CODE;
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

int countKeysInSlot(Conn *conn, int slot) {
  char buf[MAX_CMD_SIZE], *line;
  int ret;
  Reply reply;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER COUNTKEYSINSLOT %d", slot);
  ret = command(conn, buf, &reply);
  if (ret == MY_ERR_CODE) {
    freeReply(&reply);
    return ret;
  }

  line = LAST_LINE2(reply);
  ret = line == NULL ? 0 : atoi(line);
  freeReply(&reply);

  return ret;
}
