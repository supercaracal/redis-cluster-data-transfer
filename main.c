#include "./generic.h"
#include "./net.h"
#include "./command.h"
#include "./cluster.h"

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

  for (i = 0; i < ret; ++i) {
    ret = createConnection(cluster->nodes[i]);
    if (ret == MY_ERR_CODE) return ret;
  }

  freeReply(&reply);
  freeConnection(&conn);
  return MY_OK_CODE;
}

int main(int argc, char **argv) {
  int ret;
  Cluster src, dest;

  if (argc < 3 || argc > 3) {
    fprintf(stderr, "Usage: ./main [src-host:port] [dest-host:port]\n");
    exit(1);
  }

  ret = fetchClusterState(argv[1], &src);
  ASSERT(ret);

  ret = fetchClusterState(argv[2], &dest);
  ASSERT(ret);

  freeClusterState(&src);
  freeClusterState(&dest);
  exit(0);
}
