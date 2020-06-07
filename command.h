#ifndef COMMAND_H_
#define COMMAND_H_

#include "./generic.h"

int command(Conn *, const char *, Reply *);
int pipeline(Conn *, const char *, Reply *, int);
int commandWithRawData(Conn *, const void *, Reply *, int);
void freeReply(Reply *);
void printReplyLines(const Reply *);
int isKeylessCommand(const char *);

#endif // COMMAND_H_
