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

/* remove a from t and free it */
Ndbtuple*
ndbdiscard(Ndbtuple *t, Ndbtuple *a)
{
	Ndbtuple *nt;

	/* unchain a */
	for(nt = t; nt != nil; nt = nt->entry){
		if(nt->line == a)
			nt->line = a->line;
		if(nt->entry == a)
			nt->entry = a->entry;
	}

	/* a may be start of chain */
	if(t == a)
		t = a->entry;

	/* free a */
	a->entry = nil;
	ndbfree(a);

	ndbsetmalloctag(t, getcallerpc(&t));
	return t;
}
