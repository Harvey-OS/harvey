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

int
strcspn(const char *s, const char *b)
{
	const char *os;
	char map[N];

	memset(map, 0, N);
	for(;;) {
		map[*(u8*)b] = 1;
		if(*b++ == 0)
			break;
	}
	os = s;
	while(map[*(u8*)s++] == 0)
		;
	return s - os - 1;
}
