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
#include <errno.h>

int
lstat(char *name, struct stat *ans)
{
	return stat(name, ans);
}

int
symlink(char *, char *)
{
	errno = EPERM;
	return -1;
}

int
readlink(char *, char *, int )
{
	errno = EIO;
	return -1;
}
