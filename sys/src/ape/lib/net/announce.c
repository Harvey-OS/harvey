/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libnet.h>

#define NAMELEN 28

static int	nettrans(int8_t*, int8_t*, int na, int8_t*, int);

/*
 *  announce a network service.
 */
int
announce(int8_t *addr, int8_t *dir)
{
	int ctl, n, m;
	int8_t buf[3*NAMELEN];
	int8_t buf2[3*NAMELEN];
	int8_t netdir[2*NAMELEN];
	int8_t naddr[3*NAMELEN];
	int8_t *cp;

	/*
	 *  translate the address
	 */
	if(nettrans(addr, naddr, sizeof(naddr), netdir, sizeof(netdir)) < 0)
		return -1;

	/*
	 * get a control channel
	 */
	ctl = open(netdir, O_RDWR);
	if(ctl<0)
		return -1;
	cp = strrchr(netdir, '/');
	*cp = 0;

	/*
	 *  find out which line we have
	 */
	n = sprintf(buf, "%.*s/", 2*NAMELEN+1, netdir);
	m = read(ctl, &buf[n], sizeof(buf)-n-1);
	if(n<=0){
		close(ctl);
		return -1;
	}
	buf[n+m] = 0;

	/*
	 *  make the call
	 */
	n = sprintf(buf2, "announce %.*s", 2*NAMELEN, naddr);
	if(write(ctl, buf2, n)!=n){
		close(ctl);
		return -1;
	}

	/*
	 *  return directory etc.
	 */
	if(dir)
		strcpy(dir, buf);
	return ctl;
}

/*
 *  listen for an incoming call
 */
int
listen(int8_t *dir, int8_t *newdir)
{
	int ctl, n, m;
	int8_t buf[3*NAMELEN];
	int8_t *cp;

	/*
	 *  open listen, wait for a call
	 */
	sprintf(buf, "%.*s/listen", 2*NAMELEN+1, dir);
	ctl = open(buf, O_RDWR);
	if(ctl < 0)
		return -1;

	/*
	 *  find out which line we have
	 */
	strcpy(buf, dir);
	cp = strrchr(buf, '/');
	*++cp = 0;
	n = cp-buf;
	m = read(ctl, cp, sizeof(buf) - n - 1);
	if(n<=0){
		close(ctl);
		return -1;
	}
	buf[n+m] = 0;

	/*
	 *  return directory etc.
	 */
	if(newdir)
		strcpy(newdir, buf);
	return ctl;

}

/*
 *  accept a call, return an fd to the open data file
 */
int
accept(int ctl, int8_t *dir)
{
	int8_t buf[128];
	int8_t *num;
	int32_t n;

	num = strrchr(dir, '/');
	if(num == 0)
		num = dir;
	else
		num++;

	sprintf(buf, "accept %s", num);
	n = strlen(buf);
	write(ctl, buf, n); /* ignore return value, netowrk might not need accepts */

	sprintf(buf, "%s/data", dir);
	return open(buf, O_RDWR);
}

/*
 *  reject a call, tell device the reason for the rejection
 */
int
reject(int ctl, int8_t *dir, int8_t *cause)
{
	int8_t buf[128];
	int8_t *num;
	int32_t n;

	num = strrchr(dir, '/');
	if(num == 0)
		num = dir;
	else
		num++;
	sprintf(buf, "reject %s %s", num, cause);
	n = strlen(buf);
	if(write(ctl, buf, n) != n)
		return -1;
	return 0;
}

/*
 *  perform the identity translation (in case we can't reach cs)
 */
static int
identtrans(int8_t *addr, int8_t *naddr, int na, int8_t *file, int nf)
{
	int8_t reply[4*NAMELEN];
	int8_t *p;

	USED(nf);

	/* parse the network */
	strncpy(reply, addr, sizeof(reply));
	reply[sizeof(reply)-1] = 0;
	p = strchr(addr, '!');
	if(p)
		*p++ = 0;

	sprintf(file, "/net/%.*s/clone", na - sizeof("/net//clone"), reply);
	strncpy(naddr, p, na);
	naddr[na-1] = 0;

	return 1;
}

/*
 *  call up the connection server and get a translation
 */
static int
nettrans(int8_t *addr, int8_t *naddr, int na, int8_t *file, int nf)
{
	int fd;
	int8_t reply[4*NAMELEN];
	int8_t *cp;
	int32_t n;

	/*
	 *  ask the connection server
	 */
	fd = open("/net/cs", O_RDWR);
	if(fd < 0)
		return identtrans(addr, naddr, na, file, nf);
	if(write(fd, addr, strlen(addr)) < 0){
		close(fd);
		return -1;
	}
	lseek(fd, 0, 0);
	n = read(fd, reply, sizeof(reply)-1);
	close(fd);
	if(n <= 0)
		return -1;
	reply[n] = 0;

	/*
	 *  parse the reply
	 */
	cp = strchr(reply, ' ');
	if(cp == 0)
		return -1;
	*cp++ = 0;
	strncpy(naddr, cp, na);
	naddr[na-1] = 0;
	strncpy(file, reply, nf);
	file[nf-1] = 0;
	return 0;
}
