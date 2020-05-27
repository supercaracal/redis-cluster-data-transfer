#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

#define MAX_CMD_SIZE 4096
#define MIGRATE_CMD_TIMEOUT 30000

#define LOG_PROG(i) (printf("%5d slots were copied\n", i))

static int fetchClusterState(const char *str, Cluster *cluster) {
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

static int countKeysInSlot(const Conn *conn, int slot) {
  char buf[MAX_CMD_SIZE], *line;
  int ret;
  Reply reply;

  snprintf(buf, MAX_CMD_SIZE, "CLUSTER COUNTKEYSINSLOT %d", slot);
  ret = command(conn, buf, &reply);
  if (ret == MY_ERR_CODE) return ret;
  line = LAST_LINE2(reply);
  ret = line == NULL ? 0 : atoi(line);
  freeReply(&reply);

  return ret;
}

static int copyKey(const Conn *src, const Conn *dest, const char *key) {
  char buf[MAX_CMD_SIZE];
  int ret;
  Reply reply;

  snprintf(buf, MAX_CMD_SIZE, "MIGRATE %s %s %s 0 %d COPY REPLACE", dest->addr.host, dest->addr.port, key, MIGRATE_CMD_TIMEOUT);
  ret = command(src, buf, &reply);
  if (ret == MY_ERR_CODE) return ret;
  freeReply(&reply);

  return MY_OK_CODE;
}

static int migrateKeys(const Cluster *src, const Cluster *dest) {
  char buf[MAX_CMD_SIZE];
  int i, j, ret, cnt;
  Reply reply;

  for (i = 0; i < CLUSTER_SLOT_SIZE; ++i) {
    if (i > 0 && i % 1000 == 0) LOG_PROG(i);

    ret = countKeysInSlot(FIND_CONN(src, i), i);
    if (ret == MY_ERR_CODE) return ret;

    cnt = ret;
    if (cnt == 0) continue;

    snprintf(buf, MAX_CMD_SIZE, "CLUSTER GETKEYSINSLOT %d %d", i, cnt);
    ret = command(FIND_CONN(src, i), buf, &reply);
    if (ret == MY_ERR_CODE) return ret;

    for (j = 0; j < reply.i; ++j) {
      ret = copyKey(FIND_CONN(src, i), FIND_CONN(dest, i), reply.lines[j]);
      if (ret == MY_ERR_CODE) return ret;
    }

    freeReply(&reply);
  }
  LOG_PROG(CLUSTER_SLOT_SIZE);

  return MY_OK_CODE;
}

int main(int argc, char **argv) {
  int ret;
  Cluster src, dest;

  if (argc < 3 || argc > 3) {
    fprintf(stderr, "Usage: bin/exe [src-host:port] [dest-host:port]\n");
    exit(1);
  }

  ret = fetchClusterState(argv[1], &src);
  ASSERT(ret);

  ret = fetchClusterState(argv[2], &dest);
  ASSERT(ret);

  // TODO: multi thread
  ret = migrateKeys(&src, &dest);
  ASSERT(ret);

  ret = freeClusterState(&src);
  ASSERT(ret);

  ret = freeClusterState(&dest);
  ASSERT(ret);

  exit(0);
}
