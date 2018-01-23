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

int
runestrcmp(const Rune *s1, const Rune *s2)
{
	Rune c1, c2;

	do{
		c1 = *s1++;
		c2 = *s2++;
		if(c1 < c2)
			return -1;
		if(c1 > c2)
			return 1;
	}while(c1 != 0);

	return 0;
}
