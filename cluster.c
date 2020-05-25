#include "./cluster.h"

void buildSlotNodeTable(Reply *reply, int *slots) {
  int i;

  fprintf(stdout, "------------------------------------------------------------\n");
  for (i = 0; i < reply->i; ++i) fprintf(stdout, "%s\n", reply->lines[i]);
}
