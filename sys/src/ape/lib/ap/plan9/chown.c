/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include "sys9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "dir.h"

int
chown(const char *path, uid_t owner, gid_t group)
{
	int num;
	Dir d;

	_nulldir(&d);

	/* find owner, group */
	d.uid = nil;
	num = owner;
	if(!_getpw(&num, &d.uid, 0)) {
		errno = EINVAL;
		return -1;
	}

	d.gid = nil;
	num = group;
	if(!_getpw(&num, &d.gid, 0)) {
		errno = EINVAL;
		return -1;
	}

	if(_dirwstat(path, &d) < 0){
		_syserrno();
		return -1;
	}
	return 0;
}
