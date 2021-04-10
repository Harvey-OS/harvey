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

u32
umuldiv(u32 a, u32 b, u32 c)
{
	double d;

	d = ((double)a * (double)b) / (double)c;
	if(d >= 4294967296.)
		abort();
	return d;
}

i32
muldiv(i32 a, i32 b, i32 c)
{
	int s;
	i32 v;

	s = 0;
	if(a < 0) {
		s = !s;
		a = -a;
	}
	if(b < 0) {
		s = !s;
		b = -b;
	}
	if(c < 0) {
		s = !s;
		c = -c;
	}
	v = umuldiv(a, b, c);
	if(s)
		v = -v;
	return v;
}
