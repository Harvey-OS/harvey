#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libnet.h>

#define NAMELEN 28

static int	nettrans(char*, char*, int na, char*, int);

/*
 *  announce a network service.
 */
int
announce(char *addr, char *dir)
{
	int ctl, n, m;
	char buf[3*NAMELEN];
	char buf2[3*NAMELEN];
	char netdir[2*NAMELEN];
	char naddr[3*NAMELEN];
	char *cp;

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
listen(char *dir, char *newdir)
{
	int ctl, n, m;
	char buf[3*NAMELEN];
	char *cp;

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
accept(int ctl, char *dir)
{
	char buf[128];
	char *num;
	long n;

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
reject(int ctl, char *dir, char *cause)
{
	char buf[128];
	char *num;
	long n;

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
identtrans(char *addr, char *naddr, int na, char *file, int nf)
{
	char reply[4*NAMELEN];
	char *p;

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
nettrans(char *addr, char *naddr, int na, char *file, int nf)
{
	int fd;
	char reply[4*NAMELEN];
	char *cp;
	long n;

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
