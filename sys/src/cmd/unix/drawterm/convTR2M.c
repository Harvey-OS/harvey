#include "lib9.h"
#include "auth.h"

#define	CHAR(x)		*p++ = f->x
#define	SHORT(x)	p[0] = f->x; p[1] = f->x>>8; p += 2
#define	VLONG(q)	p[0] = (q); p[1] = (q)>>8; p[2] = (q)>>16; p[3] = (q)>>24; p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(p, f->x, n); p += n

int
convTR2M(Ticketreq *f, char *ap)
{
	int n;
	uchar *p;

	p = (uchar*)ap;
	CHAR(type);
	STRING(authid, NAMELEN);
	STRING(authdom, DOMLEN);
	STRING(chal, CHALLEN);
	STRING(hostid, NAMELEN);
	STRING(uid, NAMELEN);
	n = p - (uchar*)ap;
	return n;
}

