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
#include "plumb.h"

int
plumbsendtext(int fd, char *src, char *dst, char *wdir, char *data)
{
	Plumbmsg m;

	m.src = src;
	m.dst = dst;
	m.wdir = wdir;
	m.type = "text";
	m.attr = nil;
	m.ndata = strlen(data);
	m.data = data;
	return plumbsend(fd, &m);
}
