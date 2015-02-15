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

/*
 * /adm/users is
 *	id:user/group:head member:other members
 *
 * /etc/{passwd,group}
 *	name:x:nn:other stuff
 */

static int isnumber(char *s);

sniff(Biobuf *b)
{
	read first line of file into p;

	nf = getfields(p, f, nelem(f), ":");
	if(nf < 4)
		return nil;

	if(isnumber(f[0]) && !isnumber(f[2]))
		return _plan9;

	if(!isnumber(f[0]) && isnumber(f[2]))
		return _unix;

	return nil;
}


int
isnumber(char *s)
{
	char *q;

	strtol(s, &q, 10);
	return *q == '\0';
}

/* EOF */
