/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"

/*
 * BUG: errno mapping
 */
int
mkdir(const char *name, mode_t mode)
{
	int n;
	struct stat st;

	if(stat(name, &st)==0) {
		errno = EEXIST;
		return -1;
	}
	n = _CREATE(name, 0, 0x80000000|(mode&0777));
	if(n < 0)
		_syserrno();
	else{
		_CLOSE(n);
		n = 0;
	}
	return n;
}
