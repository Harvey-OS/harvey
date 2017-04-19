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
#include <bin.h>
#include <httpd.h>

char*
hlower(char *p)
{
	char c;
	char *x;

	if(p == nil)
		return p;

	for(x = p; (c = *x) != '\0'; x++)
		if(c >= 'A' && c <= 'Z')
			*x -= 'A' - 'a';
	return p;
}
