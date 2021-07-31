/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>

#include "priv.h"

Rock *_sock_rock;

Rock*
_sock_findrock(int fd, struct stat *dp)
{
	Rock *r;
	struct stat d;

	if(dp == 0)
		dp = &d;
	fstat(fd, dp);
	for(r = _sock_rock; r; r = r->next){
		if(r->inode == dp->st_ino
		&& r->dev == dp->st_dev)
			break;
	}
	return r;
}

Rock*
_sock_newrock(int fd)
{
	Rock *r;
	struct stat d;

	r = _sock_findrock(fd, &d);
	if(r == 0){
		r = malloc(sizeof(Rock));
		if(r == 0)
			return 0;
		r->dev = d.st_dev;
		r->inode = d.st_ino;
		r->other = -1;
		r->next = _sock_rock;
		_sock_rock = r;
	}
	memset(&r->raddr, 0, sizeof(r->raddr));
	memset(&r->addr, 0, sizeof(r->addr));
	r->reserved = 0;
	r->dev = d.st_dev;
	r->inode = d.st_ino;
	r->other = -1;
	return r;
}

int
_sock_data(int cfd, char *net, int domain, int stype, int protocol, Rock **rp)
{
	int n, fd;
	Rock *r;
	char name[Ctlsize];

	/* get the data file name */
	n = read(cfd, name, sizeof(name)-1);
	if(n < 0){
		close(cfd);
		errno = ENOBUFS;
		return -1;
	}
	name[n] = 0;
	n = strtoul(name, 0, 0);
	snprintf(name, sizeof name, "/net/%s/%d/data", net, n);

	/* open data file */
	fd = open(name, O_RDWR);
	close(cfd);
	if(fd < 0){
		close(cfd);
		errno = ENOBUFS;
		return -1;
	}

	/* hide stuff under the rock */
	snprintf(name, sizeof name, "/net/%s/%d/ctl", net, n);
	r = _sock_newrock(fd);
	if(r == 0){
		errno = ENOBUFS;
		close(fd);
		return -1;
	}
	if(rp)
		*rp = r;
	memset(&r->raddr, 0, sizeof(r->raddr));
	memset(&r->addr, 0, sizeof(r->addr));
	r->domain = domain;
	r->stype = stype;
	r->protocol = protocol;
	strcpy(r->ctl, name);
	return fd;
}

int
socket(int domain, int stype, int protocol)
{
	Rock *r;
	int cfd, fd, n;
	int pfd[2];
	char *net;

	switch(domain){
	case PF_INET:
		/* get a free network directory */
		switch(stype){
		case SOCK_DGRAM:
			net = "udp";
			cfd = open("/net/udp/clone", O_RDWR);
			break;
		case SOCK_STREAM:
			net = "tcp";
			cfd = open("/net/tcp/clone", O_RDWR);
			break;
		default:
			errno = EPROTONOSUPPORT;
			return -1;
		}
		if(cfd < 0){
			_syserrno();
			return -1;
		}
		return _sock_data(cfd, net, domain, stype, protocol, 0);
	case PF_UNIX:
		if(pipe(pfd) < 0){
			_syserrno();
			return -1;
		}
		r = _sock_newrock(pfd[0]);
		r->domain = domain;
		r->stype = stype;
		r->protocol = protocol;
		r->other = pfd[1];
		return pfd[0];
	default:
		errno = EPROTONOSUPPORT;
		return -1;
	}
}

int
issocket(int fd)
{
	Rock *r;

	r = _sock_findrock(fd, 0);
	return (r != 0);
}

/*
 * probably should do better than this
 */
int getsockopt(int, int, int, void *, int *)
{
	return -1;
}

int setsockopt(int, int, int, void *, int)
{
	return 0;
}

