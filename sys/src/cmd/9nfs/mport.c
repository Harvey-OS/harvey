#include "all.h"

typedef struct Rpcconn	Rpcconn;

struct Rpcconn
{
	int	data;
	int	ctl;
	Rpccall	cmd;
	Rpccall	reply;
	uchar	rpcbuf[8192];
	uchar	argbuf[8192];
};

void	putauth(char*, Auth*);
int	rpccall(Rpcconn*, int);

int	rpcdebug;

Rpcconn	r;
char *	mach;

void
main(int argc, char **argv)
{
	char addr[64], dir[64], name[128];
	char buf[33], *p;
	uchar *dataptr, *argptr;
	int i, fd, n, remport;

	ARGBEGIN{
	case 'm':
		mach = ARGF();
		break;
	case 'D':
		++rpcdebug;
		break;
	}ARGEND
	if(argc != 1)
		exits("usage");

	snprint(addr, sizeof addr, "udp!%s!111", argv[0]);
	r.data = dial(addr, 0, dir, &r.ctl);
	if(r.data < 0){
		fprint(2, "dial %s: %r\n", addr);
		exits("dial");
	}
	if(rpcdebug)
		fprint(2, "dial %s: dir=%s\n", addr, dir);
	if(fprint(r.ctl, "headers") < 0){
		fprint(2, "can't set header mode: %r\n");
		exits("headers");
	}
	sprint(name, "%s/remote", dir);
	fd = open(name, OREAD);
	if(fd < 0){
		fprint(2, "can't open %s: %r\n", name);
		exits("remote");
	}
	n = read(fd, buf, sizeof buf-1);
	if(n < 0){
		fprint(2, "can't read %s: %r\n", name);
		exits("remote");
	}
	close(fd);
	buf[n] = 0;
	p = buf;
	r.cmd.host = 0;
	for(i=0; i<4; i++, p++)
		r.cmd.host = (r.cmd.host<<8)|strtol(p, &p, 10);
	r.cmd.port = strtol(p, 0, 10);
	fprint(2, "host=%ld.%ld.%ld.%ld, port=%lud\n",
		(r.cmd.host>>24)&0xff, (r.cmd.host>>16)&0xff,
		(r.cmd.host>>8)&0xff, r.cmd.host&0xff, r.cmd.port);
	fprint(r.ctl, "disconnect");

	r.cmd.xid = time(0);
	r.cmd.mtype = CALL;
	r.cmd.rpcvers = 2;
	r.cmd.args = r.argbuf;
	if(mach)
		putauth(mach, &r.cmd.cred);

	r.cmd.prog = 100000;	/* portmapper */
	r.cmd.vers = 2;		/* vers */
	r.cmd.proc = 3;		/* getport */
	dataptr = r.cmd.args;

	PLONG(100005);		/* mount */
	PLONG(1);		/* vers */
	PLONG(IPPROTO_UDP);
	PLONG(0);

	i = rpccall(&r, dataptr-(uchar*)r.cmd.args);
	if(i != 4)
		exits("trouble");
	argptr = r.reply.results;
	remport = GLONG();
	fprint(2, "remote port = %d\n", remport);

	r.cmd.port = remport;
	r.cmd.prog = 100005;	/* mount */
	r.cmd.vers = 1;		/* vers */
	r.cmd.proc = 0;		/* null */
	dataptr = r.cmd.args;

	i = rpccall(&r, dataptr-(uchar*)r.cmd.args);
	if(i != 0)
		exits("trouble");
	fprint(2, "OK ping mount\n");

	r.cmd.prog = 100005;	/* mount */
	r.cmd.vers = 1;		/* vers */
	r.cmd.proc = 5;		/* export */
	dataptr = r.cmd.args;

	i = rpccall(&r, dataptr-(uchar*)r.cmd.args);
	fprint(2, "export: %d bytes returned\n", i);
	argptr = r.reply.results;
	while (GLONG() != 0) {
		n = GLONG();
		p = GPTR(n);
		print("%.*s\n", utfnlen(p, n), p);
		while (GLONG() != 0) {
			n = GLONG();
			p = GPTR(n);
			print("\t%.*s\n", utfnlen(p, n), p);
		}
	}

	exits(0);
}

void
putauth(char *mach, Auth *a)
{
	uchar *dataptr;
	long stamp = time(0);
	int n = strlen(mach);

	dataptr = realloc(a->data, 2*4+ROUNDUP(n)+4*4);
	a->data = dataptr;
	a->flavor = AUTH_UNIX;
	PLONG(stamp);
	PLONG(n);
	PPTR(mach, n);
	PLONG(0);
	PLONG(1);
	PLONG(1);
	PLONG(0);
	a->count = dataptr - (uchar*)a->data;
}

int
rpccall(Rpcconn *r, int narg)
{
	int n;

	r->cmd.xid++;
	n = rpcS2M(&r->cmd, narg, r->rpcbuf);
	if(rpcdebug)
		rpcprint(2, &r->cmd);
	if(write(r->data, r->rpcbuf, n) < 0){
		fprint(2, "rpc write: %r\n");
		exits("rpc");
	}
	n = read(r->data, r->rpcbuf, sizeof r->rpcbuf);
	if(n < 0){
		fprint(2, "rpc read: %r\n");
		exits("rpc");
	}
	if(rpcM2S(r->rpcbuf, &r->reply, n) != 0){
		fprint(2, "rpc reply format error\n");
		exits("rpc");
	}
	if(rpcdebug)
		rpcprint(2, &r->reply);
	if(r->reply.mtype != REPLY || r->reply.stat != MSG_ACCEPTED ||
	   r->reply.astat != SUCCESS){
		fprint(2, "rpc mtype, stat, astat = %ld, %ld, %ld\n",
			r->reply.mtype, r->reply.stat, r->reply.astat);
		exits("rpc");
	}
	return n - (((uchar *)r->reply.results) - r->rpcbuf);
}
