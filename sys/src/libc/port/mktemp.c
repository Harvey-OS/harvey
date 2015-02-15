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
mktemp(char *as)
{
	char *s;
	unsigned pid;
	int i;
	char err[ERRMAX];

	pid = getpid();
	s = as;
	while(*s++)
		;
	s--;
	while(*--s == 'X') {
		*s = pid % 10 + '0';
		pid = pid/10;
	}
	s++;
	i = 'a';
	while(access(as, 0) != -1) {
		if (i == 'z')
			return "/";
		*s = i++;
	}
	err[0] = '\0';
	errstr(err, sizeof err);	/* clear the error */
	return as;
}
