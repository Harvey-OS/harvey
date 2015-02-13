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

uint32_t
umuldiv(uint32_t a, uint32_t b, uint32_t c)
{
	double d;

	d = ((double)a * (double)b) / (double)c;
	if(d >= 4294967296.)
		abort();
	return d;
}

int32_t
muldiv(int32_t a, int32_t b, int32_t c)
{
	int s;
	int32_t v;

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
