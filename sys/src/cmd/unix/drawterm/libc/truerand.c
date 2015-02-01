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

ulong
truerand(void)
{
	ulong x;
	static int randfd = -1;

	if(randfd < 0)
		randfd = open("/dev/random", OREAD|OCEXEC);
	if(randfd < 0)
		sysfatal("can't open /dev/random");
	if(read(randfd, &x, sizeof(x)) != sizeof(x))
		sysfatal("can't read /dev/random");
	return x;
}
