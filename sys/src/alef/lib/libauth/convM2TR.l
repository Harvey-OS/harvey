#include <alef.h>
#include <auth.h>

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	VLONG(q)	q = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(f->x, p, n); p += n

void
convM2TR(byte *ap, Ticketreq *f)
{
	byte *p;

	p = ap;
	CHAR(type);
	STRING(authid, NAMELEN);
	f->authid[NAMELEN-1] = 0;
	STRING(authdom, DOMLEN);
	f->authdom[DOMLEN-1] = 0;
	STRING(chal, CHALLEN);
	STRING(hostid, NAMELEN);
	f->hostid[NAMELEN-1] = 0;
	STRING(uid, NAMELEN);
	f->uid[NAMELEN-1] = 0;
	if(p);
}
