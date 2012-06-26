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
