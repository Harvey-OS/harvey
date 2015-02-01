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
#include <venti.h>

char*
vtstrdup(char *s)
{
	int n;
	char *ss;

	if(s == nil)
		return nil;
	n = strlen(s) + 1;
	ss = vtmalloc(n);
	memmove(ss, s, n);
	return ss;
}

