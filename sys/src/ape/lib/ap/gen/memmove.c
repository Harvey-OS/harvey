/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>

void*
memmove(void *a1, const void *a2, size_t n)
{
	char *s1, *s2;
	extern void abort(void);

	if((long)n < 0)
		abort();
	if(a1 > a2)
		goto back;
	s1 = a1;
	s2 = a2;
	while(n > 0) {
		*s1++ = *s2++;
		n--;
	}
	return a1;

back:
	s1 = (char*)a1 + n;
	s2 = (char*)a2 + n;
	while(n > 0) {
		*--s1 = *--s2;
		n--;
	}
	return a1;
}

void*
memcpy(void *a1, const void *a2, size_t n)
{
	return memmove(a1, a2, n);
}
