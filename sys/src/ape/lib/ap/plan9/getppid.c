/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

pid_t
getppid(void)
{
	int n, f;
	char ppidbuf[15];

	f = open("#c/ppid", 0);
	n = read(f, ppidbuf, sizeof ppidbuf);
	if(n < 0)
		errno = EINVAL;
	else
		n = atoi(ppidbuf);
	close(f);
	return n;
}
