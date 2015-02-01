/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * turn raw (no echo, etc.) on and off.
 * ptyfs is gone, so don't even try tcsetattr, etc.
 */
#define _POSIX_SOURCE
#define _RESEARCH_SOURCE

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <libv.h>

static int ctlfd = -1;

/* fd is ignored */

tty_echooff(int)
{
	if(ctlfd >= 0)
		return 0;
	ctlfd = open("/dev/consctl", O_WRONLY);
	if(ctlfd < 0)
		return -1;
	write(ctlfd, "rawon", 5);
	return 0;
}

tty_echoon(int)
{
	if(ctlfd >= 0){
		write(ctlfd, "rawoff", 6);
		close(ctlfd);
		ctlfd = -1;
		return 0;
	}
	return -1;
}
