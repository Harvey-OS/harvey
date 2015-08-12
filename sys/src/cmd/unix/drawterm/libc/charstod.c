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
#include "fmtdef.h"

/*
 * Reads a floating-point number by interpreting successive characters
 * returned by (*f)(vp).  The last call it makes to f terminates the
 * scan, so is not a character in the number.  It may therefore be
 * necessary to back up the input stream up one byte after calling charstod.
 */

double
fmtcharstod(int (*f)(void *), void *vp)
{
	double num, dem;
	int neg, eneg, dig, exp, c;

	num = 0;
	neg = 0;
	dig = 0;
	exp = 0;
	eneg = 0;

	c = (*f)(vp);
	while(c == ' ' || c == '\t')
		c = (*f)(vp);
	if(c == '-' || c == '+') {
		if(c == '-')
			neg = 1;
		c = (*f)(vp);
	}
	while(c >= '0' && c <= '9') {
		num = num * 10 + c - '0';
		c = (*f)(vp);
	}
	if(c == '.')
		c = (*f)(vp);
	while(c >= '0' && c <= '9') {
		num = num * 10 + c - '0';
		dig++;
		c = (*f)(vp);
	}
	if(c == 'e' || c == 'E') {
		c = (*f)(vp);
		if(c == '-' || c == '+') {
			if(c == '-') {
				dig = -dig;
				eneg = 1;
			}
			c = (*f)(vp);
		}
		while(c >= '0' && c <= '9') {
			exp = exp * 10 + c - '0';
			c = (*f)(vp);
		}
	}
	exp -= dig;
	if(exp < 0) {
		exp = -exp;
		eneg = !eneg;
	}
	dem = __fmtpow10(exp);
	if(eneg)
		num /= dem;
	else
		num *= dem;
	if(neg)
		return -num;
	return num;
}
