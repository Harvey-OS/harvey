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
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
utime(const char *path, const struct utimbuf *times)
{
	int n;
	Dir nd;
	time_t curt;

	_nulldir(&nd);
	if(times == 0) {
		curt = time(0);
		nd.atime = curt;
		nd.mtime = curt;
	} else {
		nd.atime = times->actime;
		nd.mtime = times->modtime;
	}
	n = _dirwstat(path, &nd);
	if(n < 0)
		_syserrno();
	return n;
}
