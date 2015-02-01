/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <errno.h>
#include <unistd.h>
#include "lib.h"
#include "sys9.h"

ssize_t
write(int d, const void *buf, size_t nbytes)
{
	int n;

	if(d<0 || d>=OPEN_MAX || !(_fdinfo[d].flags&FD_ISOPEN)){
		errno = EBADF;
		return -1;
	}
	if(_fdinfo[d].oflags&O_APPEND)
		_SEEK(d, 0, 2);
	n = _WRITE(d, buf, nbytes);
	if(n < 0)
		_syserrno();
	return n;
}
