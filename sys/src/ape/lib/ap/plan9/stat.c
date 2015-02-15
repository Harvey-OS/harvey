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
#include <stdlib.h>
#include "sys9.h"
#include "dir.h"

int
stat(const char *path, struct stat *buf)
{
	Dir *d;

	if((d = _dirstat(path)) == nil){
		_syserrno();
		return -1;
	}
	_dirtostat(buf, d, 0);
	free(d);

	return 0;
}
