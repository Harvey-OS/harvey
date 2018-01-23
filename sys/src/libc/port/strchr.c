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

char*
strchr(const char *s, int c)
{
	char c0 = c;
	char c1;

	if(c == 0) {
		while(*s++)
			;
		return (char*)s-1;
	}

	while((c1 = *s++) != 0)
		if(c1 == c0)
			return (char*)s-1;
	return 0;
}
