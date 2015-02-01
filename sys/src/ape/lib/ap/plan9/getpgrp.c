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
#include <stdio.h>
#include "sys9.h"
#include "lib.h"

pid_t
getpgrp(void)
{
	int n, f, pid;
	char pgrpbuf[15], fname[30];

	pid = getpid();
	sprintf(fname, "/proc/%d/noteid", pid);
	f = open(fname, 0);
	n = read(f, pgrpbuf, sizeof pgrpbuf);
	if(n < 0)
		_syserrno();
	else
		n = atoi(pgrpbuf);
	close(f);
	return n;
}
