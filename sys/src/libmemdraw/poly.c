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

void
mempoly(Memimage *dst, Point *vert, int nvert, int end0, int end1, int radius, Memimage *src, Point sp, int op)
{
	int i, e0, e1;
	Point d;

	if(nvert < 2)
		return;
	d = subpt(sp, vert[0]);
	for(i=1; i<nvert; i++){
		e0 = e1 = Enddisc;
		if(i == 1)
			e0 = end0;
		if(i == nvert-1)
			e1 = end1;
		memline(dst, vert[i-1], vert[i], e0, e1, radius, src, addpt(d, vert[i-1]), op);
	}
}
