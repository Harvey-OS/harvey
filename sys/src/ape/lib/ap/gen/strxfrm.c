/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>

size_t
strxfrm(char *s1, const char *s2, size_t n)
{
	/*
	 * BUG: supposed to transform s2 to a canonical form
	 * so that strcmp can be used instead of strcoll, but
	 * our strcoll just uses strcmp.
	 */

	size_t xn = strlen(s2);
	if(n > xn)
		n = xn;
	memcpy(s1, s2, n);
	return xn;
}
