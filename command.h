#ifndef COMMAND_H_
#define COMMAND_H_

#include "./generic.h"

int command(Conn *, const char *, Reply *);
int pipeline(Conn *, const char *, Reply *, int n);
void freeReply(Reply *);
void printReplyLines(const Reply *);

#endif // COMMAND_H_
