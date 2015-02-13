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
strtok_r(int8_t *s, const int8_t *b, int8_t **last)
{
	int8_t map[N], *os;

	memset(map, 0, N);
	while(*b)
		map[*(unsigned char*)b++] = 1;
	if(s == 0)
		s = *last;
	while(map[*(unsigned char*)s++])
		;
	if(*--s == 0)
		return 0;
	os = s;
	while(map[*(unsigned char*)s] == 0)
		if(*s++ == 0) {
			*last = s-1;
			return os;
		}
	*s++ = 0;
	*last = s;
	return os;
}

int8_t*
strtok(int8_t *s, const int8_t *b)
{
	static int8_t *under_rock;

	return strtok_r(s, b, &under_rock);
}
