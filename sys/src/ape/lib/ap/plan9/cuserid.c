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

/*
 * BUG: supposed to be for effective uid,
 * but plan9 doesn't have that concept
 */
char *
cuserid(char *s)
{
	char *logname;
	static char buf[L_cuserid];

	if((logname = getlogin()) == NULL)
		return(NULL);
	if(s == 0)
		s = buf;
	strncpy(s, logname, sizeof buf);
	return(s);
}
