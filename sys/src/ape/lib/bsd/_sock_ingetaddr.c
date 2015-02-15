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
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "priv.h"

void
_sock_ingetaddr(Rock *r, struct sockaddr_in *ip, int *alen, char *a)
{
	int n, fd;
	char *p;
	char name[Ctlsize];

	/* get remote address */
	strcpy(name, r->ctl);
	p = strrchr(name, '/');
	strcpy(p+1, a);
	fd = open(name, O_RDONLY);
	if(fd >= 0){
		n = read(fd, name, sizeof(name)-1);
		if(n > 0){
			name[n] = 0;
			p = strchr(name, '!');
			if(p){
				*p++ = 0;
				ip->sin_family = AF_INET;
				ip->sin_port = htons(atoi(p));
				ip->sin_addr.s_addr = inet_addr(name);
				if(alen)
					*alen = sizeof(struct sockaddr_in);
			}
		}
		close(fd);
	}

}
