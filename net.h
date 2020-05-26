#ifndef NET_H_
#define NET_H_

#include "./generic.h"

int createConnection(Conn *);
int createConnectionFromStr(const char *, Conn *);
int freeConnection(Conn *);

#endif // NET_H_
