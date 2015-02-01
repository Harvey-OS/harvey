/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>

char*
sysname(void)
{
	int f, n;
	static char b[128];

	if(b[0])
		return b;

	f = open("#c/sysname", 0);
	if(f >= 0) {
		n = read(f, b, sizeof(b)-1);
		if(n > 0)
			b[n] = 0;
		close(f);
	}
	return b;
}
