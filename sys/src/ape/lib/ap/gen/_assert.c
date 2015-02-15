/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

void
_assert(char *f, unsigned line)
{
	char buf[20], *p, *s = &buf[20];
	write(2, "assertion failed: file ", 23);
	for(p = f; *p; p++) continue;
	write(2, f, p-f);
	write(2, ":", 7);
	*--s = '\n';
	do *--s = line%10 + '0'; while (line /= 10);
	write(2, s, &buf[20] - s);
	abort();
	
}
