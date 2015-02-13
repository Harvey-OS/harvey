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

#define	N	256

int32_t
strcspn(int8_t *s, int8_t *b)
{
	int8_t map[N], *os;

	memset(map, 0, N);
	for(;;) {
		map[*(uint8_t*)b] = 1;
		if(*b++ == 0)
			break;
	}
	os = s;
	while(map[*(uint8_t*)s++] == 0)
		;
	return s - os - 1;
}
