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
int	notrans;

void
usage(void){
	fprint(2, "usage: %s [-in] [network-dir]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int justinterfaces = 0;

	ARGBEGIN{
	case 'n':
		notrans = 1;
		break;
	case 'i':
		justinterfaces = 1;
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

	nstat("tcp", pip);
	nstat("udp", pip);
	nstat("rudp", pip);
	nstat("il", pip);

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
	static char buf[Ndbvlen];
	Ndbtuple *t;

	if(notrans)
		return p;
	t = csgetval(netroot, "port", p, net, buf);
	if(t)
		ndbfree(t);
	if(buf[0] == 0)
		return p;
	return buf;
}

void
pip(char *net, Dir *db)
{
	int n, fd;
	char buf[128], *p;
	Ndbtuple *tp;
	char dname[Ndbvlen];

	if(strcmp(db->name, "clone") == 0)
		return;
	if(strcmp(db->name, "stats") == 0)
		return;

	snprint(buf, sizeof buf, "%s/%s/%s/ctl", netroot, net, db->name);

	sprint(buf, "%s/%s/%s/status", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;

	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		return;
	buf[n] = 0;
	close(fd);

	p = strchr(buf, ' ');
	if(p != 0)
		*p = 0;
	
	Bprint(&out, "%-4s %-4s %-10s %-12s ", net, db->name, db->uid, buf);

	sprint(buf, "%s/%s/%s/local", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0) {
		Bprint(&out, "\n");
		return;
	}
	n = read(fd, buf, sizeof(buf));
	if(n < 0) {
		Bprint(&out, "\n");
		return;
	}
	buf[n-1] = 0;
	close(fd);
	p = strchr(buf, '!');
	if(p == 0) {
		Bprint(&out, "\n");
		return;
	}
	*p = '\0';
	Bprint(&out, "%-10s ", getport(net, p+1));

	sprint(buf, "%s/%s/%s/remote", netroot, net, db->name);
	fd = open(buf, OREAD);
	if(fd < 0) {
		print("\n");
		return;
	}
	n = read(fd, buf, sizeof(buf));
	if(n < 0) {
		print("\n");
		return;
	}
	buf[n-1] = 0;
	close(fd);
	p = strchr(buf, '!');
	*p++ = '\0';

	if(notrans){
		Bprint(&out, "%-10s %s\n", getport(net, p), buf);
		return;
	}
	tp = csgetval(netroot, "ip", buf, "dom", dname);
	if(tp)
		ndbfree(tp);
	if(dname[0] == 0) {
		Bprint(&out, "%-10s %s\n", getport(net, p), buf);
		return;
	}
	Bprint(&out, "%-10s %s\n", getport(net, p), dname);
	Bflush(&out);
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
			Bprint(&out, "%-12s %5d %-*I %5.5M %-*I %8lud %8lud %8lud %8lud\n",
				nip->dev, nip->mtu, 
				l, lifc->ip, lifc->mask, l, lifc->net,
				nip->pktin, nip->pktout,
				nip->errin, nip->errout);
	}
	Bflush(&out);
}
