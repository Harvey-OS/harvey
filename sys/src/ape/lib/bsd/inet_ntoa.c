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
#include <stdlib.h>
#include <stdio.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

char*
inet_ntoa(struct in_addr in)
{
	static char s[18];
	unsigned char *p;	

	p = (unsigned char*)&in.s_addr;
	snprintf(s, sizeof s, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return s;
}
