#include "all.h"
#include <ndb.h>

static int	alarmflag;

static int	Iconv(Fmt*);
static void	openudp(int);
static void	cachereply(Rpccall*, void*, int);
static int	replycache(int, Rpccall*, long (*)(int, void*, long));
static void	udpserver(int, Progmap*);
static void	tcpserver(int, Progmap*);
static void	getendpoints(Udphdr*, char*);
static long	readtcp(int, void*, long);
static long	writetcp(int, void*, long);
static int	servemsg(int, long (*)(int, void*, long), long (*)(int, void*, long),
		int, Progmap*);
void	(*rpcalarm)(void);
int	rpcdebug;
int	rejectall;
int	p9debug;

int	nocache;

uchar	buf[9000];
uchar	rbuf[9000];
uchar	resultbuf[9000];

static int tcp;

char *commonopts = "[-9CDrtv]";			/* for usage() messages */

/*
 * this recognises common, nominally rcp-related options.
 * they may not take arguments.
 */
int
argopt(int c)
{
	switch(c){
	case '9':
		++p9debug;
		return 0;
	case 'C':
		++nocache;
		return 0;
	case 'D':
		++rpcdebug;
		return 0;
	case 'r':
		++rejectall;
		return 0;
	case 't':
		tcp = 1;
		return 0;
	case 'v':
		++chatty;
		return 0;
	default:
		return -1;
	}
}

/*
 * all option parsing is now done in (*pg->init)(), which can call back
 * here to argopt for common options.
 */
void
server(int argc, char **argv, int myport, Progmap *progmap)
{
	Progmap *pg;

	fmtinstall('I', Iconv);
	fmtinstall('F', fcallfmt);
	fmtinstall('D', dirfmt);

	switch(rfork(RFNOWAIT|RFENVG|RFNAMEG|RFNOTEG|RFFDG|RFPROC)){
	case -1:
		panic("fork");
	default:
		_exits(0);
	case 0:
		break;
	}

	switch(rfork(RFMEM|RFPROC)){
	case 0:
		for(;;){
			sleep(30*1000);
			alarmflag = 1;
		}
	case -1:
		sysfatal("rfork: %r");
	}

	for(pg=progmap; pg->init; pg++)
		(*pg->init)(argc, argv);
	if(tcp)
		tcpserver(myport, progmap);
	else
		udpserver(myport, progmap);
}

static void
udpserver(int myport, Progmap *progmap)
{
	char service[128];
	char data[128];
	char devdir[40];
	int ctlfd, datafd;

	snprint(service, sizeof service, "udp!*!%d", myport);
	ctlfd = announce(service, devdir);
	if(ctlfd < 0)
		panic("can't announce %s: %r\n", service);
	if(fprint(ctlfd, "headers") < 0)
		panic("can't set header mode: %r\n");

	snprint(data, sizeof data, "%s/data", devdir);
	datafd = open(data, ORDWR);
	if(datafd < 0)
		panic("can't open udp data: %r\n");
	close(ctlfd);

	chatsrv(0);
	clog("%s: listening to port %d\n", argv0, myport);
	while (servemsg(datafd, read, write, myport, progmap) >= 0)
		continue;
	exits(0);
}

static void
tcpserver(int myport, Progmap *progmap)
{
	char adir[40];
	char ldir[40];
	char ds[40];
	int actl, lctl, data;

	snprint(ds, sizeof ds, "tcp!*!%d", myport);
	chatsrv(0);
	actl = -1;
	for(;;){
		if(actl < 0){
			actl = announce(ds, adir);
			if(actl < 0){
				clog("%s: listening to tcp port %d\n",
					argv0, myport);
				clog("announcing: %r");
				break;
			}
		}
		lctl = listen(adir, ldir);
		if(lctl < 0){
			close(actl);
			actl = -1;
			continue;
		}
		switch(fork()){
		case -1:
			clog("%s!%d: %r\n", argv0, myport);
			/* fall through */
		default:
			close(lctl);
			continue;
		case 0:
			close(actl);
			data = accept(lctl, ldir);
			close(lctl);
			if(data < 0)
				exits(0);

			/* pretend it's udp; fill in Udphdr */
			getendpoints((Udphdr*)buf, ldir);

			while (servemsg(data, readtcp, writetcp, myport,
			    progmap) >= 0)
				continue;
			close(data);
			exits(0);
		}
	}
	exits(0);
}

static int
servemsg(int fd, long (*readmsg)(int, void*, long), long (*writemsg)(int, void*, long),
		int myport, Progmap * progmap)
{
	int i, n, nreply;
	Rpccall rcall, rreply;
	int vlo, vhi;
	Progmap *pg;
	Procmap *pp;
	char errbuf[ERRMAX];

	if(alarmflag){
		alarmflag = 0;
		if(rpcalarm)
			(*rpcalarm)();
	}
	n = (*readmsg)(fd, buf, sizeof buf);
	if(n < 0){
		errstr(errbuf, sizeof errbuf);
		if(strcmp(errbuf, "interrupted") == 0)
			return 0;
		clog("port %d: error: %s\n", myport, errbuf);
		return -1;
	}
	if(n == 0){
		clog("port %d: EOF\n", myport);
		return -1;
	}
	if(rpcdebug == 1)
		fprint(2, "%s: rpc from %d.%d.%d.%d/%d\n",
			argv0, buf[12], buf[13], buf[14], buf[15],
			(buf[32]<<8)|buf[33]);
	i = rpcM2S(buf, &rcall, n);
	if(i != 0){
		clog("udp port %d: message format error %d\n",
			myport, i);
		return 0;
	}
	if(rpcdebug > 1)
		rpcprint(2, &rcall);
	if(rcall.mtype != CALL)
		return 0;
	if(replycache(fd, &rcall, writemsg))
		return 0;
	nreply = 0;
	rreply.host = rcall.host;
	rreply.port = rcall.port;
	rreply.lhost = rcall.lhost;
	rreply.lport = rcall.lport;
	rreply.xid = rcall.xid;
	rreply.mtype = REPLY;
	if(rcall.rpcvers != 2){
		rreply.stat = MSG_DENIED;
		rreply.rstat = RPC_MISMATCH;
		rreply.rlow = 2;
		rreply.rhigh = 2;
		goto send_reply;
	}
	if(rejectall){
		rreply.stat = MSG_DENIED;
		rreply.rstat = AUTH_ERROR;
		rreply.authstat = AUTH_TOOWEAK;
		goto send_reply;
	}
	i = n - (((uchar *)rcall.args) - buf);
	if(rpcdebug > 1)
		fprint(2, "arg size = %d\n", i);
	rreply.stat = MSG_ACCEPTED;
	rreply.averf.flavor = 0;
	rreply.averf.count = 0;
	rreply.results = resultbuf;
	vlo = 0x7fffffff;
	vhi = -1;
	for(pg=progmap; pg->pmap; pg++){
		if(pg->progno != rcall.prog)
			continue;
		if(pg->vers == rcall.vers)
			break;
		if(pg->vers < vlo)
			vlo = pg->vers;
		if(pg->vers > vhi)
			vhi = pg->vers;
	}
	if(pg->pmap == 0){
		if(vhi < 0)
			rreply.astat = PROG_UNAVAIL;
		else{
			rreply.astat = PROG_MISMATCH;
			rreply.plow = vlo;
			rreply.phigh = vhi;
		}
		goto send_reply;
	}
	for(pp = pg->pmap; pp->procp; pp++)
		if(rcall.proc == pp->procno){
			if(rpcdebug > 1)
				fprint(2, "process %d\n", pp->procno);
			rreply.astat = SUCCESS;
			nreply = (*pp->procp)(i, &rcall, &rreply);
			goto send_reply;
		}
	rreply.astat = PROC_UNAVAIL;
send_reply:
	if(nreply >= 0){
		i = rpcS2M(&rreply, nreply, rbuf);
		if(rpcdebug > 1)
			rpcprint(2, &rreply);
		(*writemsg)(fd, rbuf, i);
		cachereply(&rreply, rbuf, i);
	}
	return 0;
}

static void
getendpoint(char *dir, char *file, uchar *addr, uchar *port)
{
	int fd, n;
	char buf[128];
	char *sys, *serv;

	sys = serv = 0;

	snprint(buf, sizeof buf, "%s/%s", dir, file);
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv){
				*serv++ = 0;
				serv = strdup(serv);
			}
			sys = strdup(buf);
		}
		close(fd);
	}
	if(serv == 0)
		serv = strdup("unknown");
	if(sys == 0)
		sys = strdup("unknown");
	parseip(addr, sys);
	n = atoi(serv);
	hnputs(port, n);
}

/* set Udphdr values from protocol dir local & remote files */
static void
getendpoints(Udphdr *ep, char *dir)
{
	getendpoint(dir, "local", ep->laddr, ep->lport);
	getendpoint(dir, "remote", ep->raddr, ep->rport);
}

static long
readtcp(int fd, void *vbuf, long blen)
{
	uchar mk[4];
	int n, m, sofar;
	ulong done;
	char *buf;

	buf = vbuf;
	buf += Udphdrsize;
	blen -= Udphdrsize;

	done = 0;
	for(sofar = 0; !done; sofar += n){
		m = readn(fd, mk, 4);
		if(m < 4)
			return 0;
		done = (mk[0]<<24)|(mk[1]<<16)|(mk[2]<<8)|mk[3];
		m = done & 0x7fffffff;
		done &= 0x80000000;
		if(m > blen-sofar)
			return -1;
		n = readn(fd, buf+sofar, m);
		if(m != n)
			return 0;
	}
	return sofar + Udphdrsize;
}

static long
writetcp(int fd, void *vbuf, long len)
{
	char *buf;

	buf = vbuf;
	buf += Udphdrsize;
	len -= Udphdrsize;

	buf -= 4;
	buf[0] = 0x80 | (len>>24);
	buf[1] = len>>16;
	buf[2] = len>>8;
	buf[3] = len;
	len += 4;
	return write(fd, buf, len);
}
/*
 *long
 *niwrite(int fd, void *buf, long count)
 *{
 *	char errbuf[ERRLEN];
 *	long n;
 *
 *	for(;;){
 *		n = write(fd, buf, count);
 *		if(n < 0){
 *			errstr(errbuf);
 *			if(strcmp(errbuf, "interrupted") == 0)
 *				continue;
 *			clog("niwrite error: %s\n", errbuf);
 *			werrstr(errbuf);
 *		}
 *		break;
 *	}
 *	return n;
 *}
 */
long
niwrite(int fd, void *buf, long n)
{
//	int savalarm;

// 	savalarm = alarm(0);
	n = write(fd, buf, n);
// 	if(savalarm > 0)
//		alarm(savalarm);
	return n;
}

typedef struct Namecache	Namecache;
struct Namecache {
	char dom[256];
	ulong ipaddr;
	Namecache *next;
};

Namecache *dnscache;

static Namecache*
domlookupl(void *name, int len)
{
	Namecache *n, **ln;

	if(len >= sizeof(n->dom))
		return nil;

	for(ln=&dnscache, n=*ln; n; ln=&(*ln)->next, n=*ln) {
		if(strncmp(n->dom, name, len) == 0 && n->dom[len] == 0) {
			*ln = n->next;
			n->next = dnscache;
			dnscache = n;
			return n;
		}
	}
	return nil;
}

static Namecache*
domlookup(void *name)
{
	return domlookupl(name, strlen(name));
}

static Namecache*
iplookup(ulong ip)
{
	Namecache *n, **ln;

	for(ln=&dnscache, n=*ln; n; ln=&(*ln)->next, n=*ln) {
		if(n->ipaddr == ip) {
			*ln = n->next;
			n->next = dnscache;
			dnscache = n;
			return n;
		}
	}
	return nil;
}

static Namecache*
addcacheentry(void *name, int len, ulong ip)
{
	Namecache *n;

	if(len >= sizeof(n->dom))
		return nil;

	n = malloc(sizeof(*n));
	if(n == nil)
		return nil;
	strncpy(n->dom, name, len);
	n->dom[len] = 0;
	n->ipaddr = ip;
	n->next = dnscache;
	dnscache = n;
	return nil;
}

int
getdnsdom(ulong ip, char *name, int len)
{
	char buf[128];
	Namecache *nc;
	char *p;

	if(nc=iplookup(ip)) {
		strncpy(name, nc->dom, len);
		name[len-1] = 0;
		return 0;
	}
	clog("getdnsdom: %I\n", ip);
	snprint(buf, sizeof buf, "%I", ip);
	p = csgetvalue("/net", "ip", buf, "dom", nil);
	if(p == nil)
		return -1;
	strncpy(name, p, len-1);
	name[len] = 0;
	free(p);
	addcacheentry(name, strlen(name), ip);
	return 0;
}

int
getdom(ulong ip, char *dom, int len)
{
	int i;
	static char *prefix[] = { "", "gate-", "fddi-", "u-", 0 };
	char **pr;

	if(getdnsdom(ip, dom, len)<0)
		return -1;

	for(pr=prefix; *pr; pr++){
		i = strlen(*pr);
		if(strncmp(dom, *pr, i) == 0) {
			memmove(dom, dom+i, len-i);
			break;
		}
	}
	return 0;
}

#define	MAXCACHE	64

static Rpccache *head, *tail;
static int	ncache;

static void
cachereply(Rpccall *rp, void *buf, int len)
{
	Rpccache *cp;

	if(nocache)
		return;

	if(ncache >= MAXCACHE){
		if(rpcdebug)
			fprint(2, "%s: drop  %I/%ld, xid %uld, len %d\n",
				argv0, tail->host,
				tail->port, tail->xid, tail->n);
		tail = tail->prev;
		free(tail->next);
		tail->next = 0;
		--ncache;
	}
	cp = malloc(sizeof(Rpccache)+len-4);
	if(cp == 0){
		clog("cachereply: malloc %d failed\n", len);
		return;
	}
	++ncache;
	cp->prev = 0;
	cp->next = head;
	if(head)
		head->prev = cp;
	else
		tail = cp;
	head = cp;
	cp->host = rp->host;
	cp->port = rp->port;
	cp->xid = rp->xid;
	cp->n = len;
	memmove(cp->data, buf, len);
	if(rpcdebug)
		fprint(2, "%s: cache %I/%ld, xid %uld, len %d\n",
			argv0, cp->host, cp->port, cp->xid, cp->n);
}

static int
replycache(int fd, Rpccall *rp, long (*writemsg)(int, void*, long))
{
	Rpccache *cp;

	for(cp=head; cp; cp=cp->next)
		if(cp->host == rp->host &&
		   cp->port == rp->port &&
		   cp->xid == rp->xid)
			break;
	if(cp == 0)
		return 0;
	if(cp->prev){	/* move to front */
		cp->prev->next = cp->next;
		if(cp->next)
			cp->next->prev = cp->prev;
		else
			tail = cp->prev;
		cp->prev = 0;
		cp->next = head;
		head->prev = cp;
		head = cp;
	}
	(*writemsg)(fd, cp->data, cp->n);
	if(rpcdebug)
		fprint(2, "%s: reply %I/%ld, xid %uld, len %d\n",
			argv0, cp->host, cp->port, cp->xid, cp->n);
	return 1;
}

static int
Iconv(Fmt *f)
{
	char buf[16];
	ulong h;

	h = va_arg(f->args, ulong);
	snprint(buf, sizeof buf, "%ld.%ld.%ld.%ld",
		(h>>24)&0xff, (h>>16)&0xff,
		(h>>8)&0xff, h&0xff);
	return fmtstrcpy(f, buf);
}
