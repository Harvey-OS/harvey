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
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>

int
memlsetrefresh(Memimage *i, Refreshfn fn, void *ptr)
{
	Memlayer *l;

	l = i->layer;
	if(l->refreshfn!=nil && fn!=nil){	/* just change functions */
		l->refreshfn = fn;
		l->refreshptr = ptr;
		return 1;
	}

	if(l->refreshfn == nil){	/* is using backup image; just free it */
		freememimage(l->save);
		l->save = nil;
		l->refreshfn = fn;
		l->refreshptr = ptr;
		return 1;
	}

	l->save = allocmemimage(i->r, i->chan);
	if(l->save == nil)
		return 0;
	/* easiest way is just to update the entire save area */
	l->refreshfn(i, i->r, l->refreshptr);
	l->refreshfn = nil;
	l->refreshptr = nil;
	return 1;
}
