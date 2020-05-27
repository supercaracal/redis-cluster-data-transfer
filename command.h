#ifndef COMMAND_H_
#define COMMAND_H_

#include "./generic.h"

int command(const Conn *, const char *, Reply *);
void freeReply(Reply *);
void printReplyLines(const Reply *);

#endif // COMMAND_H_
