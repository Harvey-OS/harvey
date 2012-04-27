#include <u.h>
#include <libc.h>
#include <oventi.h>
#include "session.h"

void vtDumpSome(Packet*);

void
vtDebug(VtSession *s, char *fmt, ...)
{
	va_list arg;

	if(!s->debug)
		return;

	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
}

void
vtDebugMesg(VtSession *z, Packet *p, char *s)
{
	int op;
	int tid;
	int n;
	uchar buf[100], *b;


	if(!z->debug)
		return;
	n = packetSize(p);
	if(n < 2) {
		fprint(2, "runt packet%s", s);
		return;
	}
	b = packetPeek(p, buf, 0, 2);
	op = b[0];
	tid = b[1];

	fprint(2, "%c%d[%d] %d", ((op&1)==0)?'R':'Q', op, tid, n);
	vtDumpSome(p);
	fprint(2, "%s", s);
}

void
vtDumpSome(Packet *pkt)
{
	int printable;
	int i, n;
	char buf[200], *q, *eq;
	uchar data[32], *p;

	n = packetSize(pkt);
	printable = 1;
	q = buf;
	eq = buf + sizeof(buf);
	q = seprint(q, eq, "(%d) '", n);

	if(n > sizeof(data))
		n = sizeof(data);
	p = packetPeek(pkt, data, 0, n);
	for(i=0; i<n && printable; i++)
		if((p[i]<32 && p[i] !='\n' && p[i] !='\t') || p[i]>127)
				printable = 0;
	if(printable) {
		for(i=0; i<n; i++)
			q = seprint(q, eq, "%c", p[i]);
	} else {
		for(i=0; i<n; i++) {
			if(i>0 && i%4==0)
				q = seprint(q, eq, " ");
			q = seprint(q, eq, "%.2X", p[i]);
		}
	}
	seprint(q, eq, "'");
	fprint(2, "%s", buf);
}
