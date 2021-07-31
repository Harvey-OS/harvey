#include <alef.h>

intern int
call(byte *clone, byte *dest, int *cfdp, byte *dir, byte *local)
{
	int fd, cfd;
	int n;
	byte name[3*NAMELEN+5];
	byte data[3*NAMELEN+10];
	byte *p;

	cfd = open(clone, ORDWR);
	if(cfd < 0)
		return -1;

	/* get directory name */
	n = read(cfd, name, sizeof(name)-1);
	if(n < 0){
		close(cfd);
		return -1;
	}
	name[n] = 0;
	sprint(name, "%d", strtoui(name, nil, 0));
	p = strrchr(clone, '/');
	*p = 0;
	if(dir)
		sprint(dir, "%.*s/%.*s", 2*NAMELEN+1, clone, NAMELEN, name);

	sprint(data, "%.*s/%.*s/data", 2*NAMELEN+1, clone, NAMELEN, name);

	/* set local side (port number, for example) if we need to */
	if(local)
		sprint(name, "connect %.*s %.*s", 2*NAMELEN, dest, 2*NAMELEN, local);
	else
		sprint(name, "connect %.*s", 2*NAMELEN, dest);	/* connect */

	if(write(cfd, name, strlen(name)) < 0){
		close(cfd);
		return -1;
	}

	/* open data connection */
	fd = open(data, ORDWR);
	if(fd < 0){
		close(cfd);
		return -1;
	}
	if(cfdp)
		*cfdp = cfd;
	else
		close(cfd);
	return fd;
}

int
dial(byte *dest, byte *local, byte *dir, int *cfdp)
{
	byte net[128];
	byte clone[NAMELEN+12];
	byte *p;
	int n, fd, rv;

	/* go for a standard form net!... */
	p = strchr(dest, '!');
	if(p == nil){
		sprint(net, "net!%.*s", sizeof(net)-5, dest);
	} else {
		strncpy(net, dest, sizeof(net)-1);
		net[sizeof(net)-1] = 0;
	}

	/* call the connection server */
	fd = open("/net/cs", ORDWR);
	if(fd < 0){
		/* no connection server, don't translate */
		p = strchr(net, '!');
		*p++ = 0;
		sprint(clone, "/net/%s/clone", net);
		return call(clone, p, cfdp, dir, local);
	}

	/*
	 *  send dest to connection to translate
	 */
	if(write(fd, net, strlen(net)) < 0){
		close(fd);
		return -1;
	}

	/*
	 *  loop through each address from the connection server till
	 *  we get one that works.
	 */
	rv = -1;
	seek(fd, 0, 0);
	while((n = read(fd, net, sizeof(net) - 1)) > 0){
		net[n] = 0;
		p = strchr(net, ' ');
		if(p == nil)
			continue;
		*p++ = 0;
		rv = call(net, p, cfdp, dir, local);
		if(rv >= 0)
			break;
	}
	close(fd);
	return rv;
}
