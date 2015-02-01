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
#include <ndb.h>

/* replace a in t with b, the line structure in b is lost, c'est la vie */
Ndbtuple*
ndbsubstitute(Ndbtuple *t, Ndbtuple *a, Ndbtuple *b)
{
	Ndbtuple *nt;

	if(a == b){
		ndbsetmalloctag(t, getcallerpc(&t));
		return t;
	}
	if(b == nil){
		t = ndbdiscard(t, a);
		ndbsetmalloctag(t, getcallerpc(&t));
		return t;
	}

	/* all pointers to a become pointers to b */
	for(nt = t; nt != nil; nt = nt->entry){
		if(nt->line == a)
			nt->line = b;
		if(nt->entry == a)
			nt->entry = b;
	}

	/* end of b chain points to a's successors */
	for(nt = b; nt->entry; nt = nt->entry)
		nt->line = nt->entry;
	nt->line = a->line;
	nt->entry = a->entry;

	a->entry = nil;
	ndbfree(a);

	if(a == t){
		ndbsetmalloctag(b, getcallerpc(&t));
		return b;
	}else{
		ndbsetmalloctag(t, getcallerpc(&t));
		return t;
	}
}
