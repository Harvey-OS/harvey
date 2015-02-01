/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "dir.h"

int
ftruncate(int fd, off_t length)
{
	Dir d;

	if(length < 0){
		errno = EINVAL;
		return -1;
	}
	_nulldir(&d);
	d.length = length;
	if(_dirfwstat(fd, &d) < 0){
		_syserrno();
		return -1;
	}
	return 0;
}
