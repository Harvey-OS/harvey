/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

char*
mktemp(char *template)
{
	int n;
	long x;
	char *p;
	int c;
	struct stat stbuf;

	n = strlen(template);
	p = template+n-6;
	if (n < 6 || strcmp(p, "XXXXXX") != 0) {
		*template = 0;
	} else {
		x = getpid() % 100000;
		sprintf(p, "%05d", x);
		p += 5;
		for(c = 'a'; c <= 'z'; c++) {
			*p = c;
			if (stat(template, &stbuf) < 0)
				return template;
		}
		*template = 0;
	}
	return template;
}
