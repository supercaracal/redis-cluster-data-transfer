#ifndef NET_H_
#define NET_H_

#include "./generic.h"

int createConnection(const char *, Conn *);
void freeConnection(Conn *);

#endif // NET_H_
