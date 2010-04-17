#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

void	pip(char*, Dir*);
void	nstat(char*, void (*)(char*, Dir*));
void	pipifc(void);

Biobuf	out;
char	*netroot;
char *proto[20];
int nproto;
int	notrans;

void
usage(void)
{
	fprint(2, "usage: %s [-in] [-p proto] [network-dir]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int justinterfaces = 0;
	int i, tot, fd;
	Dir *d;
	char buf[128];

	ARGBEGIN{
	case 'i':
		justinterfaces = 1;
		break;
	case 'n':
		notrans = 1;
		break;
	case 'p':
		if(nproto >= nelem(proto))
			sysfatal("too many protos");
		proto[nproto++] = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	netroot = "/net";
	switch(argc){
	case 0:
		break;
	case 1:
		netroot = argv[0];
		break;
	default:
		usage();
	}

	Binit(&out, 1, OWRITE);

	if(justinterfaces){
		pipifc();
		exits(0);
	}

	if(nproto){
		for(i=0; i<nproto; i++)
			nstat(proto[i], pip);
	}else{
		fd = open(netroot, OREAD);
		if(fd < 0)
			sysfatal("open %s: %r", netroot);

		tot = dirreadall(fd, &d);
		for(i=0; i<tot; i++){
			if(strcmp(d[i].name, "ipifc") == 0)
				continue;
			snprint(buf, sizeof buf, "%s/%s/0/local", netroot, d[i].name);
			if(access(buf, 0) >= 0)
				nstat(d[i].name, pip);
		}
	}
	exits(0);
}

void
nstat(char *net, void (*f)(char*, Dir*))
{
	int fdir, i, tot;
	Dir *dir;
	char buf[128];

	snprint(buf, sizeof buf, "%s/%s", netroot, net);
	fdir = open(buf, OREAD);
	if(fdir < 0)
		return;

	tot = dirreadall(fdir, &dir);
	for(i = 0; i < tot; i++) {
		(*f)(net, &dir[i]);
		Bflush(&out);
	}
	free(dir);
	close(fdir);
}

char*
getport(char *net, char *p)
{
	static char port[10];

	strncpy(port, p, sizeof(port)-1);
	port[sizeof(port)-1] = 0;
	if(notrans || (p = csgetvalue(netroot, "port", p, net, nil)) == nil)
		return port;
	strncpy(port, p, sizeof(port)-1);
	port[sizeof(port)-1] = 0;
	free(p);
	return port;
}

void
pip(char *net, Dir *db)
{
	int n, fd;
	char buf[128], *p;
	char *dname;

	if(strcmp(db->name, "clone") == 0)
		return;
	if(strcmp(db->name, "stats") == 0)
		return;

	snprint(buf, sizeof buf, "%s/%s/%s/status", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n < 0)
		return;
	buf[n] = 0;

	p = strchr(buf, ' ');
	if(p != 0)
		*p = 0;
	p = strrchr(buf, '\n');
	if(p != 0)
		*p = 0;
	Bprint(&out, "%-4s %-4s %-10s %-12s ", net, db->name, db->uid, buf);

	snprint(buf, sizeof buf, "%s/%s/%s/local", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0) {
		Bprint(&out, "\n");
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n < 0) {
		Bprint(&out, "\n");
		return;
	}
	buf[n-1] = 0;
	p = strchr(buf, '!');
	if(p == 0) {
		Bprint(&out, "\n");
		return;
	}
	*p = '\0';
	Bprint(&out, "%-10s ", getport(net, p+1));

	snprint(buf, sizeof buf, "%s/%s/%s/remote", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0) {
		print("\n");
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n < 0) {
		print("\n");
		return;
	}
	buf[n-1] = 0;
	p = strchr(buf, '!');
	if(p != nil)
		*p++ = '\0';

	if(notrans){
		Bprint(&out, "%-10s %s\n", getport(net, p), buf);
		return;
	}
	dname = csgetvalue(netroot, "ip", buf, "dom", nil);
	if(dname == nil) {
		Bprint(&out, "%-10s %s\n", getport(net, p), buf);
		return;
	}
	Bprint(&out, "%-10s %s\n", getport(net, p), dname);
	Bflush(&out);
	free(dname);
}

void
pipifc(void)
{
	Ipifc *ip, *nip;
	Iplifc *lifc;
	char buf[100];
	int l, i;

	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);

	ip = readipifc(netroot, nil, -1);

	l = 7;
	for(nip = ip; nip; nip = nip->next){
		for(lifc = nip->lifc; lifc; lifc = lifc->next){
			i = snprint(buf, sizeof buf, "%I", lifc->ip);
			if(i > l)
				l = i;
			i = snprint(buf, sizeof buf, "%I", lifc->net);
			if(i > l)
				l = i;
		}
	}

	for(nip = ip; nip; nip = nip->next){
		for(lifc = nip->lifc; lifc; lifc = lifc->next)
			Bprint(&out, "%-12s %5d %-*I %5M %-*I %8lud %8lud %8lud %8lud\n",
				nip->dev, nip->mtu, 
				l, lifc->ip, lifc->mask, l, lifc->net,
				nip->pktin, nip->pktout,
				nip->errin, nip->errout);
	}
	Bflush(&out);
}
