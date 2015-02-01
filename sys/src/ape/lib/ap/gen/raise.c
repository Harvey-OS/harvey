/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* not a posix function, but implemented with posix kill, getpid */

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

int
raise(int sig)
{
	return kill(getpid(), sig);
}
