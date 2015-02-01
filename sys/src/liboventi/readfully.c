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
#include <oventi.h>
#include "session.h"

int
vtFdReadFully(int fd, uchar *p, int n)
{
	int nn;

	while(n > 0) {
		nn = vtFdRead(fd, p, n);
		if(nn <= 0)
			return 0;
		n -= nn;
		p += nn;
	}
	return 1;
}
