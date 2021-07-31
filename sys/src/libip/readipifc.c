#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <ip.h>

static Ipifc**
_readipifc(char *file, Ipifc **l, int index)
{
	int i, n, fd;
	char buf[4*1024];
	char *f[200];
	Ipifc *ifc;
	Iplifc *lifc, **ll;

	/* read the file */
	fd = open(file, OREAD);
	if(fd < 0)
		return l;
	n = 0;
	while((i = read(fd, buf+n, sizeof(buf)-1-n)) > 0 && n < sizeof(buf) - 1)
		n += i;
	buf[n] = 0;
	close(fd);
	n = tokenize(buf, f, nelem(f));
	if(n < 2)
		return l;

	/* allocate new interface */
	*l = ifc = mallocz(sizeof(Ipifc), 1);
	if(ifc == nil)
		return l;
	l = &ifc->next;
	ifc->index = index;
	i = 0;

	if(strcmp(f[i], "device") == 0){
		/* new format */
		i++;
		strncpy(ifc->dev, f[i++], sizeof(ifc->dev));
		ifc->dev[sizeof(ifc->dev)-1] = 0;
		i++;
		ifc->mtu = strtoul(f[i++], nil, 10);
		i++;
		ifc->sendra6 = atoi(f[i++]);
		i++;
		ifc->recvra6 = atoi(f[i++]);
		i++;
		ifc->rp.mflag = atoi(f[i++]);
		i++;
		ifc->rp.oflag = atoi(f[i++]);
		i++;
		ifc->rp.maxraint = atoi(f[i++]);
		i++;
		ifc->rp.minraint = atoi(f[i++]);
		i++;
		ifc->rp.linkmtu = atoi(f[i++]);
		i++;
		ifc->rp.reachtime = atoi(f[i++]);
		i++;
		ifc->rp.rxmitra = atoi(f[i++]);
		i++;
		ifc->rp.ttl = atoi(f[i++]);
		i++;
		ifc->rp.routerlt = atoi(f[i++]);
		i++;
		ifc->pktin = strtoul(f[i++], nil, 10);
		i++;
		ifc->pktout = strtoul(f[i++], nil, 10);
		i++;
		ifc->errin = strtoul(f[i++], nil, 10);
		i++;
		ifc->errout = strtoul(f[i++], nil, 10);
	
		ll = &ifc->lifc;
		while(n-i >= 5){
			/* allocate new local address */
			*ll = lifc = mallocz(sizeof(Iplifc), 1);
			ll = &lifc->next;
	
			parseip(lifc->ip, f[i++]);
			parseipmask(lifc->mask, f[i++]);
			parseip(lifc->net, f[i++]);
			lifc->validlt = strtoul(f[i++], nil, 10);
			lifc->preflt = strtoul(f[i++], nil, 10);
		}
	} else {
		/* old format */
		strncpy(ifc->dev, f[i++], sizeof(ifc->dev));
		ifc->dev[sizeof(ifc->dev)-1] = 0;
		ifc->mtu = strtoul(f[i++], nil, 10);

		ll = &ifc->lifc;
		while(n-i >= 7){
			/* allocate new local address */
			*ll = lifc = mallocz(sizeof(Iplifc), 1);
			ll = &lifc->next;
	
			parseip(lifc->ip, f[i++]);
			parseipmask(lifc->mask, f[i++]);
			parseip(lifc->net, f[i++]);
			ifc->pktin = strtoul(f[i++], nil, 10);
			ifc->pktout = strtoul(f[i++], nil, 10);
			ifc->errin = strtoul(f[i++], nil, 10);
			ifc->errout = strtoul(f[i++], nil, 10);
		}
	}

	return l;
}

static void
_freeifc(Ipifc *ifc)
{
	Ipifc *next;
	Iplifc *lnext, *lifc;

	if(ifc == nil)
		return;
	for(; ifc; ifc = next){
		next = ifc->next;
		for(lifc = ifc->lifc; lifc; lifc = lnext){
			lnext = lifc->next;
			free(lifc);
		}
		free(ifc);
	}
}

Ipifc*
readipifc(char *net, Ipifc *ifc, int index)
{
	int fd, i, n;
	Dir *dir;
	char directory[128];
	char buf[128];
	Ipifc **l;

	_freeifc(ifc);

	l = &ifc;
	ifc = nil;

	if(net == 0)
		net = "/net";
	snprint(directory, sizeof(directory), "%s/ipifc", net);

	if(index >= 0){
		snprint(buf, sizeof(buf), "%s/%d/status", directory, index);
		_readipifc(buf, l, index);
	} else {
		fd = open(directory, OREAD);
		if(fd < 0)
			return nil;
		n = dirreadall(fd, &dir);
		close(fd);
	
		for(i = 0; i < n; i++){
			if(strcmp(dir[i].name, "clone") == 0)
				continue;
			if(strcmp(dir[i].name, "stats") == 0)
				continue;
			snprint(buf, sizeof(buf), "%s/%s/status", directory, dir[i].name);
			l = _readipifc(buf, l, atoi(dir[i].name));
		}
		free(dir);
	}

	return ifc;
}
