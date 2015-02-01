/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <sys/types.h>
#include <grp.h>
#include <unistd.h>

/*
 * BUG: assumes group that is same as user name
 * is the one wanted (plan 9 has no "current group")
 */
gid_t
getgid(void)
{
	struct group *g;
	g = getgrnam(getlogin());
	return g? g->gr_gid : 1;
}

gid_t
getegid(void)
{
	return getgid();
}
