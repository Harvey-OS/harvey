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
#include <string.h>
#include <time.h>

/* socket extensions */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

int
rresvport(int *p)
{
	int fd;
	short i;
	struct	sockaddr_in in;
	static int next;

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd < 0)
		return -1;
	i = 600 + ((getpid()+next++)%(1024-600));
	memset(&in, 0, sizeof(in));
	in.sin_family = AF_INET;
	in.sin_port = htons(i);
printf("in.sin_port = %d\n", in.sin_port);
	if(bind(fd, &in, sizeof(in)) < 0){
		close(fd);
		return -1;
	}
	if(p)
		*p = i;
	return fd;
}
