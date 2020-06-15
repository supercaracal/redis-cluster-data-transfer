#ifndef COMMAND_RAW_H_
#define COMMAND_RAW_H_

#include "./net.h"
#include "./command.h"

#define NEED_MORE_REPLY -2

int commandWithRawData(Conn *, const void *, int, Reply *, int);
int readRemainedReplyLines(Conn *, Reply *, int);

#ifdef TEST
int PublicForTestParseRawReply(const char *, int, Reply *);
#endif

#endif  // COMMAND_RAW_H_
