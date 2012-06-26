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
getsockname(int fd, struct sockaddr *addr, int *alen)
{
	Rock *r;
	int i;
	struct sockaddr_in *lip;
	struct sockaddr_un *lunix;

	r = _sock_findrock(fd, 0);
	if(r == 0){
		errno = ENOTSOCK;
		return -1;
	}

	switch(r->domain){
	case PF_INET:
		lip = (struct sockaddr_in*)addr;
		_sock_ingetaddr(r, lip, alen, "local");
		break;
	case PF_UNIX:
		lunix = (struct sockaddr_un*)&r->addr;
		i = &lunix->sun_path[strlen(lunix->sun_path)] - (char*)lunix;
		memmove(addr, lunix, i);
		*alen = i;
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
	return 0;
}
