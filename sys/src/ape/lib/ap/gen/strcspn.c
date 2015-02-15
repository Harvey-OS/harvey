/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>

#define	N	256

size_t
strcspn(const char *s, const char *b)
{
	char map[N], *os;

	memset(map, 0, N);
	for(;;) {
		map[*(unsigned char*)b] = 1;
		if(*b++ == 0)
			break;
	}
	os = (char *)s;
	while(map[*(unsigned char*)s++] == 0)
		;
	return s - os - 1;
}
