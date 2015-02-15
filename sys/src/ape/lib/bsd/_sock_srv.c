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
#include <fcntl.h>
#include <errno.h>

/* socket extensions */
#include <sys/uio.h>
#include <sys/socket.h>

#include "priv.h"

/* we can't avoid overrunning npath because we don't know how big it is. */
void
_sock_srvname(char *npath, char *path)
{
	char *p;

	strcpy(npath, "/srv/UD.");
	p = strrchr(path, '/');
	if(p == 0)
		p = path;
	else
		p++;
	strcat(npath, p);
}

int
_sock_srv(char *path, int fd)
{
	int sfd;
	char msg[8+256+1];

	/* change the path to something in srv */
	_sock_srvname(msg, path);

	/* remove any previous instance */
	unlink(msg);

	/* put the fd in /srv and then close it */
	sfd = creat(msg, 0666);
	if(sfd < 0){
		close(fd);
		_syserrno();
		return -1;
	}
	snprintf(msg, sizeof msg, "%d", fd);
	if(write(sfd, msg, strlen(msg)) < 0){
		_syserrno();
		close(sfd);
		close(fd);
		return -1;
	}
	close(sfd);
	close(fd);
	return 0;
}
