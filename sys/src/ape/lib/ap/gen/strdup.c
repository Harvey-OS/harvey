/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

char*
strdup(char *p)
{
	int n;
	char *np;

	n = strlen(p)+1;
	np = malloc(n);
	if(np)
		memmove(np, p, n);
	return np;
}
