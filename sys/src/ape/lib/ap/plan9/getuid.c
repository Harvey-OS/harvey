/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

uid_t
getuid(void)
{
	struct passwd *p;
	p = getpwnam(getlogin());
	return p? p->pw_uid : 1;
}

uid_t
geteuid(void)
{
	return getuid();
}
