/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>

#include "priv.h"

int
sendto(int fd, void *a, int n, int flags, void *, int)
{
	/* actually, should do connect if not done already */
	return send(fd, a, n, flags);
}

int
recvfrom(int fd, void *a, int n, int flags,
	void *from, int *fromlen)
{
	if(getsockname(fd, from, fromlen) < 0)
		return -1;
	return recv(fd, a, n, flags);
}
