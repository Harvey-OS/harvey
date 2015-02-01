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
#include <errno.h>

int
dup(int oldd)
{
	return fcntl(oldd, F_DUPFD, 0);
}

int
dup2(int oldd, int newd)
{
	if(newd < 0 || newd >= OPEN_MAX){
		errno = EBADF;
		return -1;
	}
	if(oldd == newd && _fdinfo[newd].flags&FD_ISOPEN)
		return newd;
	close(newd);
	return fcntl(oldd, F_DUPFD, newd);
}
