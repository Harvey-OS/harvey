#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>

void	pip(char*, char*);
void	pdk(char*, char*);
void	nstat(char*, void (*)(char*, char*));

Ndb 	*ndb;
Biobuf	out;

void
main(void)
{
	ndb = ndbopen(0);

	Binit(&out, 1, OWRITE);

	nstat("tcp", pip);
	nstat("udp", pip);
	nstat("il", pip);
	nstat("dk", pdk);
	exits(0);
}

void
nstat(char *net, void (*f)(char*, char*))
{
	int fdir, i, n, tot;
	char dir[500*DIRLEN], *mem;

	sprint(dir, "/net/%s", net);
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

void
pdk(char *net, char *entry)
{
	Dir db;
	int n, fd;
	char buf[128], *p, *t;

	if(strcmp(entry, "clone") == 0)
		return;

	sprint(buf, "/net/%s/%s/ctl", net, entry);
	if(dirstat(buf, &db) < 0)
		return;

	sprint(buf, "/net/%s/%s/status", net, entry);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		return;
	buf[n] = 0;
	close(fd);

	p = strchr(buf, ' ');
	if(p == 0)
		return;
	p = strchr(p+1, ' ');
	if(p == 0)
		return;
	t = strchr(p+1, ' ');
	if(t == 0)
		return;
	*t = 0;
	Bprint(&out, "%-4s %-4s %-10s %-12s ", net, entry, db.uid, p);

	sprint(buf, "/net/%s/%s/local", net, entry);
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
	p = strchr(buf, '!');
	if(p == 0) {
		Bprint(&out, "\n");
		return;
	}
	Bprint(&out, "%-10s ", p+1);

	sprint(buf, "/net/%s/%s/remote", net, entry);
	fd = open(buf, OREAD);
	if(fd < 0) {
		Bprint(&out, "\n", buf);
		return;
	}
	n = read(fd, buf, sizeof(buf));
	if(n < 0) {
		Bprint(&out, "\n");
		return;
	}
	buf[n-1] = 0;
	Bprint(&out, "%s\n", buf);
}

char*
getport(char *net, char *p)
{
	Ndbs s;
	static Ndbtuple *t;
	static Ndbtuple *nt;

	t = ndbsearch(ndb, &s, "port", p);
	while(t){
		for(nt = t; nt; nt = nt->entry) {
			if(strcmp(nt->attr, net) == 0)
				return nt->val;
		}
		ndbfree(t);
		t = ndbsnext(&s, "port", p);
	}
	return p;
}

void
pip(char *net, char *entry)
{
	Dir db;
	Ipinfo ipi;
	int n, fd;
	char buf[128], *p, *t;

	if(strcmp(entry, "clone") == 0)
		return;

	sprint(buf, "/net/%s/%s/ctl", net, entry);
	if(dirstat(buf, &db) < 0)
		return;

	sprint(buf, "/net/%s/%s/status", net, entry);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof(buf));
	if(n < 0)
		return;
	buf[n] = 0;
	close(fd);

	p = " Dgram";
	if(strcmp(net, "udp") != 0) {
		p = strchr(buf, ' ');
		if(p == 0)
			return;
		p = strchr(p+1, ' ');
		if(p == 0)
			return;
		t = strchr(p+1, ' ');
		if(t == 0)
			return;
		*t = 0;
	}
	Bprint(&out, "%-4s %-4s %-10s %-12s ", net, entry, db.uid, p);

	sprint(buf, "/net/%s/%s/local", net, entry);
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

	sprint(buf, "/net/%s/%s/remote", net, entry);
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

	n = ipinfo(ndb, 0, buf, 0, &ipi);
	if(n < 0) {
		Bprint(&out, "%-10s %s\n", getport(net, p), buf);
		return;
	}
	Bprint(&out, "%-10s %s\n", getport(net, p), ipi.domain);
}
