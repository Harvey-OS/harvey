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

int
drawreplxy(int min, int max, int x)
{
	int sx;

	sx = (x-min)%(max-min);
	if(sx < 0)
		sx += max-min;
	return sx+min;
}

Point
drawrepl(Rectangle r, Point p)
{
	p.x = drawreplxy(r.min.x, r.max.x, p.x);
	p.y = drawreplxy(r.min.y, r.max.y, p.y);
	return p;
}
