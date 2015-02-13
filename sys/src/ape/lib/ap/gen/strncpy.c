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
strncpy(int8_t *s1, const int8_t *s2, size_t n)
{
	int i;
	int8_t *os1;

	os1 = s1;
	for(i = 0; i < n; i++)
		if((*s1++ = *s2++) == 0) {
			while(++i < n)
				*s1++ = 0;
			return os1;
		}
	return os1;
}
