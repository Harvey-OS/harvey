/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <errno.h>
#include <unistd.h>
#include "sys9.h"

pid_t
fork(void)
{
	int n;

	n = _RFORK(RFENVG|RFFDG|RFPROC);
	if(n < 0)
		_syserrno();
	if(n == 0) {
		_detachbuf();
		_sessleader = 0;
	}
	return n;
}
