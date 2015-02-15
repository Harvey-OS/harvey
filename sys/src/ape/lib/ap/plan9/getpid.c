/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

pid_t
getpid(void)
{
	int n, f;
	char pidbuf[15];

	f = _OPEN("#c/pid", 0);
	n = _READ(f, pidbuf, sizeof pidbuf);
	if(n < 0)
		_syserrno();
	else
		n = atoi(pidbuf);
	_CLOSE(f);
	return n;
}
