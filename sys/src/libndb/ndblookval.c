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
#include <bio.h>
#include <ip.h>
#include <ndb.h>

/*
 *  Look for a pair with the given attribute.  look first on the same line,
 *  then in the whole entry.
 */
Ndbtuple*
ndbfindattr(Ndbtuple *entry, Ndbtuple *line, char *attr)
{
	Ndbtuple *nt;

	/* first look on same line (closer binding) */
	for(nt = line; nt;){
		if(strcmp(attr, nt->attr) == 0)
			return nt;
		nt = nt->line;
		if(nt == line)
			break;
	}

	/* search whole tuple */
	for(nt = entry; nt; nt = nt->entry)
		if(strcmp(attr, nt->attr) == 0)
			return nt;

	return nil;
}

Ndbtuple*
ndblookval(Ndbtuple *entry, Ndbtuple *line, char *attr, char *to)
{
	Ndbtuple *t;

	t = ndbfindattr(entry, line, attr);
	if(t != nil){
		strncpy(to, t->val, Ndbvlen-1);
		to[Ndbvlen-1] = 0;
	}
	return t;
}
