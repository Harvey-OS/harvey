/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

typedef struct Arg Arg;
struct Arg
{
	int fd;
	char adir[40];
	SunSrv *srv;
};

static void
sunNetListen(void *v)
{
	int fd, lcfd;
	char ldir[40];
	Arg *a = v;

	for(;;){
		lcfd = listen(a->adir, ldir);
		if(lcfd < 0)
			break;
		fd = accept(lcfd, ldir);
		close(lcfd);
		if(fd < 0)
			continue;
		if(!sunSrvFd(a->srv, fd))
			close(fd);
	}
	free(a);
	close(a->fd);
}

int
sunSrvNet(SunSrv *srv, char *addr)
{
	Arg *a;

	a = emalloc(sizeof(Arg));
	if((a->fd = announce(addr, a->adir)) < 0)
		return -1;
	a->srv = srv;

	proccreate(sunNetListen, a, SunStackSize);
	return 0;
}

int
sunSrvAnnounce(SunSrv *srv, char *addr)
{
	if(strstr(addr, "udp!"))
		return sunSrvUdp(srv, addr);
	else
		return sunSrvNet(srv, addr);
}
