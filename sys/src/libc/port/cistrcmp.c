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
#include <ctype.h>

int
cistrcmp(const char *s1, const char *s2)
{
	unsigned int c1, c2;

	do{
		c1 = *(uint8_t*)s1++;
		c2 = *(uint8_t*)s2++;
		if(c1 == c2)
			continue;
		c1 = tolower(c1);
		c2 = tolower(c2);
		if(c1 < c2)
			return -1;
		if(c1 > c2)
			return 1;
	}while(c1 != 0);

	return 0;
}
