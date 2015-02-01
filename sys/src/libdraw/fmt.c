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
Rfmt(Fmt *f)
{
	Rectangle r;

	r = va_arg(f->args, Rectangle);
	return fmtprint(f, "%P %P", r.min, r.max);
}

int
Pfmt(Fmt *f)
{
	Point p;

	p = va_arg(f->args, Point);
	return fmtprint(f, "[%d %d]", p.x, p.y);
}

