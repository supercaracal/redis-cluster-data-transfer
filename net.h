#ifndef NET_H_
#define NET_H_

int createConnection(Conn *);
int createConnectionFromStr(const char *, Conn *);
int reconnect(Conn *);
int freeConnection(Conn *);

#endif // NET_H_
