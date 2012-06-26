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
connect(int fd, void *a, int alen)
{
	Rock *r;
	int n, cfd, nfd;
	char msg[8+256+1], file[8+256+1];
	struct sockaddr_in *lip, *rip;
	struct sockaddr_un *runix;
	static int vers;

	r = _sock_findrock(fd, 0);
	if(r == 0){
		errno = ENOTSOCK;
		return -1;
	}
	if(alen > sizeof(r->raddr)){
		errno = ENAMETOOLONG;
		return -1;
	}
	memmove(&r->raddr, a, alen);

	switch(r->domain){
	case PF_INET:
		/* set up a tcp or udp connection */
		cfd = open(r->ctl, O_RDWR);
		if(cfd < 0){
			_syserrno();
			return -1;
		}
		rip = a;
		lip = (struct sockaddr_in*)&r->addr;
		if(lip->sin_port)
			snprintf(msg, sizeof msg, "connect %s!%d%s %d",
				inet_ntoa(rip->sin_addr), ntohs(rip->sin_port),
				r->reserved ? "!r" : "",
				ntohs(lip->sin_port));
		else
			snprintf(msg, sizeof msg, "connect %s!%d%s",
				inet_ntoa(rip->sin_addr), ntohs(rip->sin_port),
				r->reserved ? "!r" : "");
		n = write(cfd, msg, strlen(msg));
		if(n < 0){
			_syserrno();
			close(cfd);
			return -1;
		}
		close(cfd);
		return 0;
	case PF_UNIX:
		/* null terminate the address */
		if(alen == sizeof(r->raddr))
			alen--;
		*(((char*)&r->raddr)+alen) = 0;

		if(r->other < 0){
			errno = EGREG;
			return -1;
		}

		/* put far end of our pipe in /srv */
		snprintf(msg, sizeof msg, "UD.%d.%d", getpid(), vers++);
		if(_sock_srv(msg, r->other) < 0){
			r->other = -1;
			return -1;
		}
		r->other = -1;

		/* tell server the /srv file to open */
		runix = (struct sockaddr_un*)&r->raddr;
		_sock_srvname(file, runix->sun_path);
		nfd = open(file, O_RDWR);
		if(nfd < 0){
			_syserrno();
			unlink(msg);
			return -1;
		}
		if(write(nfd, msg, strlen(msg)) < 0){
			_syserrno();
			close(nfd);
			unlink(msg);
			return -1;
		}
		close(nfd);

		/* wait for server to open it and then remove it */
		read(fd, file, sizeof(file));
		_sock_srvname(file, msg);
		unlink(file);
		return 0;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
}
