/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "sys9.h"

int
setpgid(pid_t pid, pid_t pgid)
{
	int n, f;
	char buf[50], fname[30];

	if(pid == 0)
		pid = getpid();
	if(pgid == 0)
		pgid = getpgrp();
	sprintf(fname, "/proc/%d/noteid", pid);
	f = open(fname, 1);
	if(f < 0) {
		errno = ESRCH;
		return -1;
	}
	n = sprintf(buf, "%d", pgid);
	n = write(f, buf, n);
	if(n < 0)
		_syserrno();
	else
		n = 0;
	close(f);
	return n;
}
