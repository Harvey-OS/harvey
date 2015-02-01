/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>

int
dirfwstat(int fd, Dir *d)
{
	uchar *buf;
	int r;

	r = sizeD2M(d);
	buf = malloc(r);
	if(buf == nil)
		return -1;
	convD2M(d, buf, r);
	r = fwstat(fd, buf, r);
	free(buf);
	return r;
}
