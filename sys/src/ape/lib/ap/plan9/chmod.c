/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

static int
seterrno(void)
{
	_syserrno();
	return -1;
}

int
chmod(const char *path, mode_t mode)
{
	Dir d, *dir;

	dir = _dirstat(path);
	if(dir == nil)
		return seterrno();
	_nulldir(&d);
	d.mode = (dir->mode & ~0777) | (mode & 0777);
	free(dir);
	if(_dirwstat(path, &d) < 0)
		return seterrno();
	return 0;
}

int
fchmod(int fd, mode_t mode)
{
	Dir d, *dir;

	dir = _dirfstat(fd);
	if(dir == nil)
		return seterrno();
	_nulldir(&d);
	d.mode = (dir->mode & ~0777) | (mode & 0777);
	free(dir);
	if(_dirfwstat(fd, &d) < 0)
		return seterrno();
	return 0;
}
