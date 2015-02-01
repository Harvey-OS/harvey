/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

/*
 * BUG: errno mapping
 */
off_t
lseek(int d, off_t offset, int whence)
{
	long long n;
	int flags;

	flags = _fdinfo[d].flags;
	if(flags&(FD_BUFFERED|FD_BUFFEREDX|FD_ISTTY)) {
		errno = ESPIPE;
		return -1;
	}
	n = _SEEK(d, offset, whence);
	if(n < 0)
		_syserrno();
	return n;
}
