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

int8_t*
strpbrk(const int8_t *s, const int8_t *b)
{
	int8_t map[N];

	memset(map, 0, N);
	for(;;) {
		map[*b] = 1;
		if(*b++ == 0)
			break;
	}
	while(map[*s++] == 0)
		;
	if(*--s)
		return (int8_t *)s;
	return 0;
}
