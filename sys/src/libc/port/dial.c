#include <u.h>
#include <libc.h>

static int
call(char *clone, char *dest, int *cfdp, char *dir, char *local)
{
	int fd, cfd;
	int n;
	char name[3*NAMELEN+5];
	char data[3*NAMELEN+10];
	char *p;

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
	p = strrchr(clone, '/');
	*p = 0;
	if(dir)
		snprint(dir, 2*NAMELEN, "%s/%s", clone, name);
	snprint(data, sizeof(data), "%s/%s/data", clone, name);

	/* set local side (port number, for example) if we need to */
	if(local){
		snprint(name, sizeof(name), "announce %s", local);
		if(write(cfd, name, strlen(name)) < 0){
			close(cfd);
			return -1;
		}
	}

	/* connect */
	snprint(name, sizeof(name), "connect %s", dest);
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
dial(char *dest, char *local, char *dir, int *cfdp)
{
	char net[128];
	char clone[NAMELEN+12];
	char *p;
	int n;
	int fd;
	int rv;

	/* go for a standard form net!... */
	p = strchr(dest, '!');
	if(p == 0){
		snprint(net, sizeof(net), "net!%s", dest);
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
		snprint(clone, sizeof(clone), "/net/%s/clone", net);
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
		if(p == 0)
			continue;
		*p++ = 0;
		rv = call(net, p, cfdp, dir, local);
		if(rv >= 0)
			break;
	}
	close(fd);
	return rv;
}
