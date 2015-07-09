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
cistrstr(char *s, char *sub)
{
	int c, csub, n;

	csub = *sub;
	if(csub == '\0')
		return s;
	if(csub >= 'A' && csub <= 'Z')
		csub -= 'A' - 'a';
	sub++;
	n = strlen(sub);
	for(; c = *s; s++){
		if(c >= 'A' && c <= 'Z')
			c -= 'A' - 'a';
		if(c == csub && cistrncmp(s+1, sub, n) == 0)
			return s;
	}
	return nil;
}
