#ifndef CONN_H
#define CONN_H

typedef enum IO {
	IN	= 0,
	OUT	= 1,
} IO;

class Conn:public Thing {
public:
	Text *pin;
	Text *net;
	IO io;
	Conn(Text *t, IO i) {pin = t; net = 0; io = i;}
	void nets(WireList *);
	void put(FILE *);
	void putm(FILE *);
	void fixup(char *);
};

class ConnList:public Vector {
public:
	ConnList(int nn):Vector(nn)	{}
	Conn *conn(int i)	{return (Conn *) a[i];}
	void nets(WireList *);
};
#endif
