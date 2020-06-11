#ifndef COMMAND_RAW_H_
#define COMMAND_RAW_H_

#include "./net.h"

int commandWithRawData(Conn *, const void *, Reply *, int);
int tryToReadFromSocket(Conn *, void *);
int tryToWriteToSocket(Conn *, const void *, int);

#endif  // COMMAND_RAW_H_
