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

ssize_t
read(int d, void *buf, size_t nbytes)
{
	int n;

	if(nbytes <= 0)
		return 0;
	if(buf == 0){
		sysfatal("read failed: %r");
	}

	n = pread(d, buf, nbytes, ~0LL);
	if(n < 0)
		sysfatal("read failed: %r");;

	return n;
}
