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
#include <thread.h>
#include <mouse.h>
#include <frame.h>

#define	CHUNK	16
#define	ROUNDUP(n)	((n+CHUNK)&~(CHUNK-1))

uchar *
_frallocstr(Frame *f, unsigned n)
{
	uchar *p;

	p = malloc(ROUNDUP(n));
	if(p == 0)
		drawerror(f->display, "out of memory");
	return p;
}

void
_frinsure(Frame *f, int bn, unsigned n)
{
	Frbox *b;
	uchar *p;

	b = &f->box[bn];
	if(b->nrune < 0)
		drawerror(f->display, "_frinsure");
	if(ROUNDUP(b->nrune) > n)	/* > guarantees room for terminal NUL */
		return;
	p = _frallocstr(f, n);
	b = &f->box[bn];
	memmove(p, b->ptr, NBYTE(b)+1);
	free(b->ptr);
	b->ptr = p;
}
