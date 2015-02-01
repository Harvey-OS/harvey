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

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int h_errno;

struct hostent*
gethostbyaddr(void *addr, int len, int type)
{
	unsigned long y;
	struct in_addr x;
	unsigned char *p = addr;

	if(type != AF_INET || len != 4){
		h_errno = NO_RECOVERY;
		return 0;
	}

	y = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
	x.s_addr = htonl(y);

	return gethostbyname(inet_ntoa(x));
}
