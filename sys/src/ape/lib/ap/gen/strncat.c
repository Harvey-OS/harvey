/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>

int8_t*
strncat(int8_t *s1, const int8_t *s2, size_t n)
{
	int8_t *os1;
	int32_t nn;

	os1 = s1;
	nn = n;
	while(*s1++)
		;
	s1--;
	while(*s1++ = *s2++)
		if(--nn < 0) {
			s1[-1] = 0;
			break;
		}
	return os1;
}
