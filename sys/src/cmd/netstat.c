#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

void	pip(char*, char*);
void	pipifc(char*, char*);
void	nstat(char*, void (*)(char*, char*));

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
		nstat("ipifc", pipifc);
		exits(0);
	}

	nstat("tcp", pip);
	nstat("udp", pip);
	nstat("rudp", pip);
	nstat("il", pip);

	exits(0);
}

void
nstat(char *net, void (*f)(char*, char*))
{
	int fdir, i, n, tot;
	char dir[500*DIRLEN], *mem;

	sprint(dir, "%s/%s", netroot, net);
	fdir = open(dir, OREAD);
	if(fdir < 0)
		return;

	mem = sbrk(0);
	tot = 0;
	while((n = read(fdir, dir, sizeof dir)) > 0){
		if(brk((void*)(mem + tot + n)) == -1) {
			fprint(2, "netstat: out of memory");
			return;
		}
		memmove(mem+tot, dir, n);
		tot += n;
	}
	for(i = 0; i < tot; i += DIRLEN) {
		(*f)(net, mem+i);
		Bflush(&out);
	}
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
pip(char *net, char *entry)
{
	Dir db;
	int n, fd;
	char buf[128], *p;
	Ndbtuple *tp;
	char dname[Ndbvlen];

	if(strcmp(entry, "clone") == 0)
		return;
	if(strcmp(entry, "stats") == 0)
		return;

	sprint(buf, "%s/%s/%s/ctl", netroot, net, entry);
	if(dirstat(buf, &db) < 0)
		return;

	sprint(buf, "%s/%s/%s/status", netroot, net, entry);
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
	
	Bprint(&out, "%-4s %-4s %-10s %-12s ", net, entry, db.uid, buf);

	sprint(buf, "%s/%s/%s/local", netroot, net, entry);
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

	sprint(buf, "%s/%s/%s/remote", netroot, net, entry);
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
pipifc(char *net, char *entry)
{
	int n, fd;
	char buf[128], *p;
	char *f[9];

	if(strcmp(entry, "clone") == 0)
		return;
	if(strcmp(entry, "stats") == 0)
		return;

	sprint(buf, "%s/%s/%s/status", netroot, net, entry);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;

	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		return;
	buf[n] = 0;
	close(fd);

	n = getfields(buf, f, 9, 1, " \t\n");
	if(n < 7)
		return;
	p = strrchr(f[0], '/');
	if(p != nil)
		f[0] = p+1;

	if(n == 9)
		Bprint(&out, "%-8s %-5s %-16s %-16s %-16s %-10s %-10s %-6s %-6s\n",
			f[0], f[1], f[2], f[3], f[4],
			f[5], f[6], f[7], f[8]);
	else
		Bprint(&out, "               %-16s %-16s %-16s %-10s %-10s %-6s %-6s\n",
			f[0], f[1], f[2], f[3], f[4],
			f[5], f[6]);
}
