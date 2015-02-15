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
#include <fcntl.h>

int
creat(const char *name, mode_t mode)
{
	int n;

	n = open(name, O_WRONLY | O_CREAT | O_TRUNC, mode);
	/* no need to _syserrno; open did it already */
	return n;
}
