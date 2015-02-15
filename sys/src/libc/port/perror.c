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

void
perror(char *s)
{
	char buf[ERRMAX];

	buf[0] = '\0';
	errstr(buf, sizeof buf);
	if(s && *s)
		fprint(2, "%s: %s\n", s, buf);
	else
		fprint(2, "%s\n", buf);
}
