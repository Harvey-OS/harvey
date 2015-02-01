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

char*
strupr(char *s)
{
	char *p;

	for(p = s; *p; p++)
		if(*p >= 0 && islower(*p))
			*p = toupper(*p);
	return s;
}

char*
strlwr(char *s)
{
	char *p;

	for(p = s; *p; p++)
		if(*p >= 0 && isupper(*p))
			*p = tolower(*p);
	return s;
}
