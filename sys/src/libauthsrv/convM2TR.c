/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <authsrv.h>

#define	CHAR(x)		f->x = *p++
#define	SHORT(x)	f->x = (p[0] | (p[1]<<8)); p += 2
#define	VLONG(q)	q = (p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24)); p += 4
#define	LONG(x)		VLONG(f->x)
#define	STRING(x,n)	memmove(f->x, p, n); p += n

void
convM2TR(char *ap, Ticketreq *f)
{
	uchar *p;

	p = (uchar*)ap;
	CHAR(type);
	STRING(authid, ANAMELEN);
	f->authid[ANAMELEN-1] = 0;
	STRING(authdom, DOMLEN);
	f->authdom[DOMLEN-1] = 0;
	STRING(chal, CHALLEN);
	STRING(hostid, ANAMELEN);
	f->hostid[ANAMELEN-1] = 0;
	STRING(uid, ANAMELEN);
	f->uid[ANAMELEN-1] = 0;
	USED(p);
}
