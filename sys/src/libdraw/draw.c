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
_setdrawop(Display *d, Drawop op)
{
	uchar *a;

	if(op != SoverD){
		a = bufimage(d, 1+1);
		if(a == 0)
			return;
		a[0] = 'O';
		a[1] = op;
	}
}
		
static void
draw1(Image *dst, Rectangle *r, Image *src, Point *p0, Image *mask, Point *p1, Drawop op)
{
	uchar *a;

	_setdrawop(dst->display, op);

	a = bufimage(dst->display, 1+4+4+4+4*4+2*4+2*4);
	if(a == 0)
		return;
	if(src == nil)
		src = dst->display->black;
	if(mask == nil)
		mask = dst->display->opaque;
	a[0] = 'd';
	BPLONG(a+1, dst->id);
	BPLONG(a+5, src->id);
	BPLONG(a+9, mask->id);
	BPLONG(a+13, r->min.x);
	BPLONG(a+17, r->min.y);
	BPLONG(a+21, r->max.x);
	BPLONG(a+25, r->max.y);
	BPLONG(a+29, p0->x);
	BPLONG(a+33, p0->y);
	BPLONG(a+37, p1->x);
	BPLONG(a+41, p1->y);
}

void
draw(Image *dst, Rectangle r, Image *src, Image *mask, Point p1)
{
	draw1(dst, &r, src, &p1, mask, &p1, SoverD);
}

void
drawop(Image *dst, Rectangle r, Image *src, Image *mask, Point p1, Drawop op)
{
	draw1(dst, &r, src, &p1, mask, &p1, op);
}

void
gendraw(Image *dst, Rectangle r, Image *src, Point p0, Image *mask, Point p1)
{
	draw1(dst, &r, src, &p0, mask, &p1, SoverD);
}

void
gendrawop(Image *dst, Rectangle r, Image *src, Point p0, Image *mask, Point p1, Drawop op)
{
	draw1(dst, &r, src, &p0, mask, &p1, op);
}
