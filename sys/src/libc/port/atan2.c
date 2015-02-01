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
/*
	atan2 discovers what quadrant the angle
	is in and calls atan.
*/

double
atan2(double arg1, double arg2)
{

	if(arg1+arg2 == arg1) {
		if(arg1 >= 0)
			return PIO2;
		return -PIO2;
	}
	arg1 = atan(arg1/arg2);
	if(arg2 < 0) {
		if(arg1 <= 0)
			return arg1 + PI;
		return arg1 - PI;
	}
	return arg1;
}
