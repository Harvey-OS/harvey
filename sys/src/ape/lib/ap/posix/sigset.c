/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <signal.h>
#include <errno.h>

/*
 * sigsets are 32-bit longs.  if the 2<<(i-1) bit is on,
 * the signal #define'd as i in signal.h is inluded.
 */

static sigset_t stdsigs = SIGHUP|SIGINT|SIGQUIT|SIGILL|SIGABRT|SIGFPE|SIGKILL|
		SIGSEGV|SIGPIPE|SIGALRM|SIGTERM|SIGUSR1|SIGUSR2;

#define BITSIG(s) (2<<(s))

int
sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}

int
sigfillset(sigset_t *set)
{
	*set = stdsigs;
	return 0;
}

int
sigaddset(sigset_t *set, int signo)
{
	int b;

	b = BITSIG(signo);
	if(!(b&stdsigs)){
		errno = EINVAL;
		return -1;
	}
	*set |= b;
	return 0;
}

int
sigdelset(sigset_t *set, int signo)
{
	int b;

	b = BITSIG(signo);
	if(!(b&stdsigs)){
		errno = EINVAL;
		return -1;
	}
	*set &= ~b;
	return 0;
}

int
sigismember(sigset_t *set, int signo)
{
	int b;

	b = BITSIG(signo);
	if(!(b&stdsigs)){
		errno = EINVAL;
		return -1;
	}
	return (b&*set)? 1 : 0;
}
