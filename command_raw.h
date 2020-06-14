#ifndef COMMAND_RAW_H_
#define COMMAND_RAW_H_

#include "./net.h"
#include "./command.h"

int commandWithRawData(Conn *, const void *, Reply *, int);
int readRemainedReplyLines(Conn *, Reply *);

#endif  // COMMAND_RAW_H_
