/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>

char *
ctermid(char *s)
{
	static char buf[L_ctermid];

	if(s == 0)
		s = buf;
	strncpy(s, "/dev/cons", sizeof buf);
	return(s);
}

char *
ctermid_r(char *s)
{
	return s ? ctermid(s) : NULL;
}
