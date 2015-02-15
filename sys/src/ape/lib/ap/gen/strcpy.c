/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>
#define	N	10000

static void*
pmemccpy(void *a1, void *a2, int c, size_t n)
{
	char *s1, *s2;

	s1 = a1;
	s2 = a2;
	while(n > 0) {
		if((*s1++ = *s2++) == c)
			return s1;
		n--;
	}
	return 0;
}

char*
strcpy(char *s1, const char *s2)
{
	char *os1;

	os1 = s1;
	while(!pmemccpy(s1, s2, 0, N)) {
		s1 += N;
		s2 += N;
	}
	return os1;
}
