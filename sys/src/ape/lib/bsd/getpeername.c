/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "priv.h"

int
getpeername(int fd, struct sockaddr *addr, int *alen)
{
	Rock *r;
	int i;
	struct sockaddr_in *rip;
	struct sockaddr_un *runix;

	r = _sock_findrock(fd, 0);
	if(r == 0){
		errno = ENOTSOCK;
		return -1;
	}

	switch(r->domain){
	case PF_INET:
		rip = (struct sockaddr_in*)&r->raddr;
		memmove(addr, rip, sizeof(struct sockaddr_in));
		*alen = sizeof(struct sockaddr_in);
		break;
	case PF_UNIX:
		runix = (struct sockaddr_un*)&r->raddr;
		i = &runix->sun_path[strlen(runix->sun_path)] - (char*)runix;
		memmove(addr, runix, i);
		*alen = i;
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
	return 0;
}
