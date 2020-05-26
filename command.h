#ifndef COMMAND_H_
#define COMMAND_H_

#include "./generic.h"

int command(Conn *, const char *, Reply *);
void freeReply(Reply *);
void printReplyLines(Reply *);

#endif // COMMAND_H_
