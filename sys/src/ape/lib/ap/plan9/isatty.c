/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sys9.h"
#include "dir.h"

int
_isatty(int fd)
{
	char buf[64];

	if(_FD2PATH(fd, buf, sizeof buf) < 0)
		return 0;

	/* might be /mnt/term/dev/cons */
	return strlen(buf) >= 9 && strcmp(buf+strlen(buf)-9, "/dev/cons") == 0;
}

/* The FD_ISTTY flag is set via _isatty in _fdsetup or open */
int
isatty(int fd)
{
	if(_fdinfo[fd].flags&FD_ISTTY)
		return 1;
	else
		return 0;
}
