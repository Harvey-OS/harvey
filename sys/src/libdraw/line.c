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
line(Image *dst, Point p0, Point p1, int end0, int end1, int radius, Image *src, Point sp)
{
	lineop(dst, p0, p1, end0, end1, radius, src, sp, SoverD);
}

void
lineop(Image *dst, Point p0, Point p1, int end0, int end1, int radius, Image *src, Point sp, Drawop op)
{
	uchar *a;

	_setdrawop(dst->display, op);

	a = bufimage(dst->display, 1+4+2*4+2*4+4+4+4+4+2*4);
	if(a == 0){
		fprint(2, "image line: %r\n");
		return;
	}
	a[0] = 'L';
	BPLONG(a+1, dst->id);
	BPLONG(a+5, p0.x);
	BPLONG(a+9, p0.y);
	BPLONG(a+13, p1.x);
	BPLONG(a+17, p1.y);
	BPLONG(a+21, end0);
	BPLONG(a+25, end1);
	BPLONG(a+29, radius);
	BPLONG(a+33, src->id);
	BPLONG(a+37, sp.x);
	BPLONG(a+41, sp.y);
}
