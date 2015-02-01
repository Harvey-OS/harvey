/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * hypot -- sqrt(p*p+q*q), but overflows only if the result does.
 * See Cleve Moler and Donald Morrison,
 * ``Replacing Square Roots by Pythagorean Sums,''
 * IBM Journal of Research and Development,
 * Vol. 27, Number 6, pp. 577-581, Nov. 1983
 */

#include <u.h>
#include <libc.h>

double
hypot(double p, double q)
{
	double r, s, pfac;

	if(p < 0)
		p = -p;
	if(q < 0)
		q = -q;
	if(p < q) {
		r = p;
		p = q;
		q = r;
	}
	if(p == 0)
		return 0;
	pfac = p;
	r = q = q/p;
	p = 1;
	for(;;) {
		r *= r;
		s = r+4;
		if(s == 4)
			return p*pfac;
		r /= s;
		p += 2*r*p;
		q *= r;
		r = q/p;
	}
}
