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
#define	N	10000

char*
strcpy(char *s1, const char *s2)
{
	char *os1;

	os1 = s1;
	while(!memccpy(s1, s2, 0, N)) {
		s1 += N;
		s2 += N;
	}
	return os1;
}
