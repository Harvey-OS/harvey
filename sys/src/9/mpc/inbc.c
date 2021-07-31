#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

int
inb(int port)
{
	return *(uchar*)(port + ISAMEM);
}

ushort
ins(int port)
{
	int x = *(ushort*)(port + ISAMEM);
	return ((x&0xff) << 8) | (x>>8);
}

ulong
inl(int port)
{
	ulong x = *(ulong*)(port + ISAMEM);
//print("inl %d\n", port);
	return (x << 24) | ((x&0xff00)<<8) | ((x>>8)&0xff00) | (x>>24);
}

void
outb(int port, int val)
{
//print("outb %d %ux\n", port, val);
	*(uchar*)(port + ISAMEM) = val;
}

void
outs(int port, ushort val)
{
//print("outs %d %ux\n", port, val);
	int x = (val<<8) | (val>>8);
	*(ushort*)(port + ISAMEM) = x;
}

void
outl(int port, ulong val)
{
//print("outl %d %ulx\n", port, val);
	ulong x = (val << 24) | ((val&0xff00)<<8) | ((val>>8)&0xff00) | (val>>24);
	*(ulong*)(port + ISAMEM) = x;
}

void
insb(int port, void *buf, int len)
{
	int i;
	uchar *p, *q;

//print("insb %d %d\n", port, len);
	p = (uchar*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*q++ = *p;
	}
}

void
inss(int port, void *buf, int len)
{
	int i;
	ushort *p, *q;

//print("inss %d %d\n", port, len);
	p = (ushort*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*q++ = *p;
	}
}

void
insl(int port, void *buf, int len)
{
	int i;
	ulong *p, *q;

//print("insl %d %d\n", port, len);
	p = (ulong*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*q++ = *p;
	}
}

void
outsb(int port, void *buf, int len)
{
	int i;
	uchar *p, *q;

//print("outsb %d %d\n", port, len);
	p = (uchar*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*p = *q++;
	}
}

void
outss(int port, void *buf, int len)
{
	int i;
	ushort *p, *q;

//print("outss %d %d\n", port, len);
	p = (ushort*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*p = *q++;
	}
}

void
outsl(int port, void *buf, int len)
{
	int i;
	ulong *p, *q;

//print("outsl %d %d\n", port, len);
	p = (ulong*)(port + ISAMEM);
	q = buf;
	for(i = 0; i < len; i++){
		*p = *q++;
	}
}
