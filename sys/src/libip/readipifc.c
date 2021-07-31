#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <ip.h>

static char*
_nexttoken(char *p, char *buf, int len)
{
	int c;

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

static Ipifc**
_insertifc(Ipifc **l, char *device, int mtu, uchar *ip, uchar *mask, uchar *ipnet, int index)
{
	Ipifc *ifc;

	/* get next ipifc */
	ifc = *l;
	if(ifc == nil){
		ifc = malloc(sizeof(Ipifc));
		if(ifc == nil)
			return l;
		ifc->next = nil;
		*l = ifc;
	}

	/* add to list */
	strncpy(ifc->dev, device, sizeof(ifc->dev));
	ifc->dev[sizeof(ifc->dev)-1] = 0;
	ifc->mtu = mtu;
	ifc->index = index;
	ipmove(ifc->ip, ip);
	ipmove(ifc->mask, mask);
	ipmove(ifc->net, ipnet);
	l = &ifc->next;

	return l;
}

static Ipifc**
_readipifc(char *file, Ipifc **l, int index)
{
	int i, n, fd;
	char buf[4*1024];
	char device[64];
	char token[64];
	uchar ip[IPaddrlen];
	uchar ipnet[IPaddrlen];
	uchar mask[IPaddrlen];
	int mtu;
	char *p;

	/* read the file */
	fd = open(file, OREAD);
	if(fd < 0)
		return l;
	n = 0;
	while((i = read(fd, buf+n, sizeof(buf)-1-n)) > 0 && n < sizeof(buf) - 1)
		n += i;
	buf[n] = 0;
	close(fd);

	/* get device and mtu */
	p = _nexttoken(buf, device, sizeof(device));
	if(*device == 0)
		return l;
	p = _nexttoken(p, token, sizeof(token));
	if(*token == 0)
		return l;
	mtu = atoi(token);

	for(i = 0; i < n; i++){
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			break;
		parseip(ip, token);
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			break;
		parseipmask(mask, token);
		p = _nexttoken(p, token, sizeof(token));
		if(*token == 0)
			break;
		parseip(ipnet, token);

		/* dump counts */
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));
		p = _nexttoken(p, token, sizeof(token));

		l = _insertifc(l, device, mtu, ip, mask, ipnet, index);
	}

	// make sure we have at least one entry to go with the device
	if(i == 0)
		l = _insertifc(l, device, mtu, IPnoaddr, IPnoaddr, IPnoaddr, index);

	return l;
}

Ipifc*
readipifc(char *net, Ipifc *ifc)
{
	int fd, i, n;
	Dir dir[32];
	char directory[128];
	char buf[128];
	Ipifc **l, *nifc, *nnifc;

	l = &ifc;

	if(net == 0)
		net = "/net";
	snprint(directory, sizeof(directory), "%s/ipifc", net);
	fd = open(directory, OREAD);
	if(fd < 0)
		goto out;

	for(;;){
		n = dirread(fd, dir, sizeof(dir));
		if(n <= 0)
			break;
		n /= sizeof(Dir);
		for(i = 0; i < n; i++){
			if(strcmp(dir[i].name, "clone") == 0)
				continue;
			snprint(buf, sizeof(buf), "%s/%s/status", directory, dir[i].name);
			l = _readipifc(buf, l, atoi(dir[i].name));
		}
	}
	close(fd);

out:
	for(nifc = *l; nifc != nil; nifc = nnifc){
		nnifc = nifc->next;
		free(nifc);
	}
	*l = nil;

	return ifc;
}
