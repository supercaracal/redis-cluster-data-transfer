#ifndef COMMAND_H_
#define COMMAND_H_

#include "./generic.h"

int command(Conn *c, const char *cmd, Reply *reply);
void freeReply(Reply *reply);

#endif // COMMAND_H_
