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

void
replclipr(Image* i, int repl, Rectangle clipr)
{
	uint8_t* b;

	b = bufimage(i->display, 22);
	b[0] = 'c';
	BPLONG(b + 1, i->id);
	repl = repl != 0;
	b[5] = repl;
	BPLONG(b + 6, clipr.min.x);
	BPLONG(b + 10, clipr.min.y);
	BPLONG(b + 14, clipr.max.x);
	BPLONG(b + 18, clipr.max.y);
	i->repl = repl;
	i->clipr = clipr;
}
