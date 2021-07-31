#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <ip.h>

static char*
_nexttoken(char *p, char *buf, int len)
{
	while(isspace(*p))
		p++;
	for(;;p++){
		c = *p;
		if(isspace(c) || c == 0)
			break;
		if(len <= 1)
			continue;
		*buf++ = c;
		len--;
	}
	*buf = 0;
	return p;
}

static void
_readipifc(char *file, Ipifcs *ifcs)
{
	int i, n, fd;
	char buf[4*1024];
	char device[64];
	char token[64];
	int mtu;
	uchar ip[IPaddrlen];
	uchar ipnet[IPaddrlen];
	uchar mask[IPaddrlen];
	Ipifc *ifc;

	/* read the file */
	fd = open(file, OREAD);
	if(fd < 0)
		return;

	n = 0;
	while((i = read(fd, buf+n, sizeof(buf)-1-n)) > 0 && n < sizeof(buf) - 1)
		n += i;
	buf[n] = 0;
	close(fd);

	/* get device and mtu */
	p = _nexttoken(buf, device, sizeof(device));
	if(*device == 0)
		return;
	p = _nexttoken(p, token, sizeof(token));
	if(*token == 0)
		return;
	mtu = atoi(token);

	for(i = 0; i < n; i++){
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			return;
		parseip(ip, token);
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			return;
		parseipmask(mask, token);
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			return;
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			return;
		parseip(ipnet, token);

		/* dump counts */
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));

		/* check for dups */
		for(ifc = ifcs->first; ifc; ifc = ifc->next)
			if(strcmp(ifc->dev, device) == 0)
			if(ifc->mtu == mtu)
		if(ifc == 0)
			for(ifc = ifcs->first; ifc; ifc = ifc->next)
				if(memcmp(ip, ifc->ip, sizeof(ip)) == 0)
					break;

		if(ifc){
			memmove(ifc->ip, ip, sizeof(ip));
			memmove(ifc->mask, mask, sizeof(mask));
			memmove(ifc->net, ipnet, sizeof(ipnet));
			strncpy(ifc->dev, field[0], sizeof(ifc->dev));
			ifc->dev[sizeof(ifc->dev)-1] = 0;
			continue;
		}

		ifc = (Ipifc*)malloc(sizeof(Ipifc));
		if(ifc == 0)
			continue;

		/* add to list */
		memmove(ifc->ip, ip, sizeof(ip));
		memmove(ifc->mask, mask, sizeof(mask));
		memmove(ifc->net, ipnet, sizeof(ipnet));
		strncpy(ifc->dev, field[0], sizeof(ifc->dev));
		ifc->dev[sizeof(ifc->dev)-1] = 0;
		ifc->next = 0;
		if(ifcs->last)
			ifcs->last->next = ifc;
		else
			ifcs->first = ifc;
		ifcs->last = ifc;
	}

}

void
readipifc(char *net, Ipifcs *ifcs)
{
	int fd, i, n;
	Dir dir[32];
	char directory[128];
	char buf[128];

	if(net == 0)
		net = "/net";
	snprint(directory, sizeof(directory), "%s/ipifc", net);
	fd = open(directory, OREAD);
	if(fd < 0)
		return;

	for(;;){
		n = dirread(fd, dir, sizeof(dir));
		if(n <= 0)
			break;
		n /= sizeof(Dir);
		for(i = 0; i < n; i++){
			if(strcmp(dir[i].name, "clone") == 0)
				continue;
			snprint(buf, sizeof(buf), "%s/%s/status", directory, dir[i].name);
			_readipifc(buf, ifcs);
		}
	}
	close(fd);
}
