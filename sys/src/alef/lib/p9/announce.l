#include <alef.h>

intern int	nettrans(byte*, byte*, int na, byte*, int);

/*
 *  announce a network service.
 */
int
announce(byte *addr, byte *dir)
{
	int ctl, n, m;
	byte buf[3*NAMELEN];
	byte buf2[3*NAMELEN];
	byte netdir[2*NAMELEN];
	byte naddr[3*NAMELEN];
	byte *cp;

	/*
	 *  translate the address
	 */
	if(nettrans(addr, naddr, sizeof(naddr), netdir, sizeof(netdir)) < 0)
		return -1;

	/*
	 * get a control channel
	 */
	ctl = open(netdir, ORDWR);
	if(ctl<0)
		return -1;
	cp = strrchr(netdir, '/');
	*cp = 0;

	/*
	 *  find out which line we have
	 */
	n = sprint(buf, "%.*s/", 2*NAMELEN+1, netdir);
	m = read(ctl, &buf[n], sizeof(buf)-n-1);
	if(n<=0){
		close(ctl);
		return -1;
	}
	buf[n+m] = 0;

	/*
	 *  make the call
	 */
	n = sprint(buf2, "announce %.*s", 2*NAMELEN, naddr);
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
listen(byte *dir, byte *newdir)
{
	int ctl, n, m;
	byte buf[3*NAMELEN];
	byte *cp;

	/*
	 *  open listen, wait for a call
	 */
	sprint(buf, "%.*s/listen", 2*NAMELEN+1, dir);
	ctl = open(buf, ORDWR);
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
accept(int ctl, byte *dir)
{
	byte buf[128];
	byte *num;
	int n;

	num = strrchr(dir, '/');
	if(num == nil)
		num = dir;
	else
		num++;

	sprint(buf, "accept %s", num);
	n = strlen(buf);
	write(ctl, buf, n); /* ignore return value, netowrk might not need accepts */

	sprint(buf, "%s/data", dir);
	return open(buf, ORDWR);
}

/*
 *  reject a call, tell device the reason for the rejection
 */
int
reject(int ctl, byte *dir, byte *cause)
{
	byte buf[128];
	byte *num;
	int n;

	num = strrchr(dir, '/');
	if(num == nil)
		num = dir;
	else
		num++;
	sprint(buf, "reject %s %s", num, cause);
	n = strlen(buf);
	if(write(ctl, buf, n) != n)
		return -1;
	return 0;
}

/*
 *  perform the identity translation (in case we can't reach cs)
 */
intern int
identtrans(byte *addr, byte *naddr, int na, byte *file, int)
{
	byte reply[4*NAMELEN];
	byte *p;

	/* parse the network */
	strncpy(reply, addr, sizeof(reply));
	reply[sizeof(reply)-1] = 0;
	p = strchr(addr, '!');
	if(p)
		*p++ = 0;

	sprint(file, "/net/%.*s/clone", na - sizeof("/net//clone"), reply);
	strncpy(naddr, p, na);
	naddr[na-1] = 0;

	return 1;
}

/*
 *  call up the connection server and get a translation
 */
intern int
nettrans(byte *addr, byte *naddr, int na, byte *file, int nf)
{
	int fd;
	byte reply[4*NAMELEN];
	byte *cp;
	int n;

	/*
	 *  ask the connection server
	 */
	fd = open("/net/cs", ORDWR);
	if(fd < 0)
		return identtrans(addr, naddr, na, file, nf);
	if(write(fd, addr, strlen(addr)) < 0){
		close(fd);
		return -1;
	}
	seek(fd, 0, 0);
	n = read(fd, reply, sizeof(reply)-1);
	close(fd);
	if(n <= 0)
		return -1;
	reply[n] = 0;

	/*
	 *  parse the reply
	 */
	cp = strchr(reply, ' ');
	if(cp == nil)
		return -1;
	*cp++ = 0;
	strncpy(naddr, cp, na);
	naddr[na-1] = 0;
	strncpy(file, reply, nf);
	file[nf-1] = 0;
	return 0;
}
