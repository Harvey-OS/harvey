#include "all.h"

static int	alarmflag;

static int	Iconv(void*, Fconv*);
static void	openlisten(int);
static int	rcvalarm(void*, char*);
static void	cachereply(Rpccall*, void*, int);
static int	replycache(int, Rpccall*);

void	(*rpcalarm)(void);
int	rpcdebug;
int	rejectall;

int	udpctl;
int	udpdata;
uchar	buf[9000];
uchar	rbuf[9000];
uchar	resultbuf[9000];

void
main(int argc, char **argv)
{
	int Argc=argc; char **Argv=argv;
	int i, n, nreply;
	Rpccall rcall, rreply;
	int vlo, vhi;
	Progmap *pg;
	Procmap *pp;
	char errbuf[ERRLEN];

	fmtinstall('I', Iconv);
	switch(rfork(RFNOWAIT|RFENVG|RFNAMEG|RFNOTEG|RFFDG|RFPROC)){
	case -1:
		panic("fork");
	default:
		_exits(0);
	case 0:
		break;
	}
	ARGBEGIN{
	case 'r':
		++rejectall;
		break;
	case 'v':
		++chatty;
		break;
	case 'D':
		++rpcdebug;
		break;
	}ARGEND
	atnotify(rcvalarm, 1);
	for(pg=progmap; pg->init; pg++)
		(*pg->init)(Argc, Argv);
	openlisten(myport);
	chatsrv(0);
	clog("%s: listening to port %d\n", argv0, myport);
	for(;;){
		if(alarmflag){
			alarmflag = 0;
			if(rpcalarm)
				(*rpcalarm)();
		}
		n = read(udpdata, buf, sizeof buf);
		if(n < 0){
			errstr(errbuf);
			if(strcmp(errbuf, "interrupted") == 0)
				continue;
			clog("udp port %d: error: %s\n", myport, errbuf);
			break;
		}
		if(n == 0){
			clog("udp port %d: EOF\n", myport);
			break;
		}
		if(rpcdebug == 1)
			fprint(2, "%s: call  %d.%d.%d.%d/%d\n",
				argv0, buf[0], buf[1], buf[2], buf[3],
				(buf[4]<<8)|buf[5]);
		i = rpcM2S(buf, &rcall, n);
		if(i != 0){
			clog("udp port %d: message format error %d\n",
				myport, i);
			continue;
		}
		if(rpcdebug > 1)
			rpcprint(2, &rcall);
		if(rcall.mtype != CALL)
			continue;
		if(replycache(udpdata, &rcall))
			continue;
		nreply = 0;
		rreply.host = rcall.host;
		rreply.port = rcall.port;
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
			write(udpdata, rbuf, i);
			cachereply(&rreply, rbuf, i);
		}
	}
	exits(0);
}

static int
rcvalarm(void *ureg, char *msg)
{
	USED(ureg);
	if(strcmp(msg, "alarm") == 0){
		++alarmflag;
		return 1;
	}
	return 0;
}

static void
openlisten(int port)
{
	char service[128];
	char data[128];
	char devdir[40];

	sprint(service, "udp!*!%d", port);
	udpctl = announce(service, devdir);
	if(udpctl < 0)
		panic("can't announce %s: %r\n", service);
	if(fprint(udpctl, "headers") < 0)
		panic("can't set header mode: %r\n");

	sprint(data, "%s/data", devdir);

	udpdata = open(data, ORDWR);
	if(udpdata < 0)
		panic("can't open udp data: %r\n");
}

long
niread(int fd, void *buf, long count)
{
	char errbuf[ERRLEN];
	long n;

	for(;;){
		n = read(fd, buf, count);
		if(n < 0){
			errstr(errbuf);
			if(strcmp(errbuf, "interrupted") == 0)
				continue;
			werrstr(errbuf);
		}
		break;
	}
	return n;
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
	int savalarm;

	savalarm = alarm(0);
	n = write(fd, buf, n);
	if(savalarm > 0)
		alarm(savalarm);
	return n;
}

ulong
getip(void *name, int len)
{
	char *p, *dot, buf[32];
	int n, pipefd[2], addr;

	if(pipe(pipefd) < 0)
		return -1;
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		perror("fork");
		break;
	case 0:
		close(pipefd[0]);
		dup(pipefd[1],1);
		p = sbrk(len+1);
		memmove(p, name, len);
		p[len] = 0;
		dot = strchr(p, '.');
		execl("/bin/ndb/query", "ndb/query",
			(dot ? "dom" : "sys"), p, "ip", 0);
		perror("exec /bin/ndb/query");
		_exits("exec");
	}
	close(pipefd[1]);
	n = niread(pipefd[0], buf, sizeof buf);
	close(pipefd[0]);
	if(n <= 0)
		return -1;
	p = buf;
	addr = strtol(buf, &p, 10)<<24;
	addr |= strtol(p+1, &p, 10)<<16;
	addr |= strtol(p+1, &p, 10)<<8;
	addr |= strtol(p+1, 0, 10);
	return addr;
}

int
getdom(ulong ip, uchar *name, int len)
{
	char buf[32];
	int n, pipefd[2];

	if(pipe(pipefd) < 0)
		return -1;
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		perror("fork");
		break;
	case 0:
		close(pipefd[0]);
		dup(pipefd[1],1);
		sprint(buf, "%d.%d.%d.%d",
			ip>>24, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
		execl("/bin/ndb/query", "ndb/query",
			"ip", buf, "dom", 0);
		perror("exec /bin/ndb/query");
		_exits("exec");
	}
	close(pipefd[1]);
	n = niread(pipefd[0], name, len);
	close(pipefd[0]);
	if(n <= 1)
		return -1;
	name[n-1] = 0;
	return 0;
}

int
getdnsdom(ulong ip, uchar *name, int len)
{
	char buf[256], *p, *q;
	int k, n, pipefd[2];

	if(pipe(pipefd) < 0)
		return -1;
	switch(rfork(RFFDG|RFPROC|RFNOWAIT)){
	case -1:
		perror("fork");
		break;
	case 0:
		close(pipefd[0]);
		dup(pipefd[1],1);
		sprint(buf, "echo %d.%d.%d.%d ptr | ndb/dnsquery",
			ip>>24, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);
		execl("/bin/rc", "rc", "-c", buf, 0);
		perror("exec /bin/rc");
		_exits("exec");
	}
	close(pipefd[1]);
	for(n=0; n<sizeof(buf); n += k){
		k = niread(pipefd[0], buf+n, sizeof(buf)-n);
		if(k <= 0)
			break;
	}
	close(pipefd[0]);
	if(n <= 1)
		return -1;
	buf[n-1] = 0;
	p = strchr(buf, '\t');
	if(p == 0)
		return -1;
	q = strchr(++p, '\n');
	if(q == 0)
		return -1;
	*q = 0;
	strncpy((char*)name, p, len);
	return 0;
}

#define	MAXCACHE	64

static Rpccache *head, *tail;
static int	ncache;

static void
cachereply(Rpccall *rp, void *buf, int len)
{
	Rpccache *cp;

	if(ncache >= MAXCACHE){
		if(rpcdebug)
			fprint(2, "%s: drop  %I/%d, xid %ud, len %d\n", 
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
	memcpy(cp->data, buf, len);
	if(rpcdebug)
		fprint(2, "%s: cache %I/%d, xid %ud, len %d\n", 
			argv0, cp->host, cp->port, cp->xid, cp->n);
}

static int
replycache(int fd, Rpccall *rp)
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
	write(fd, cp->data, cp->n);
	if(rpcdebug)
		fprint(2, "%s: reply %I/%d, xid %ud, len %d\n", 
			argv0, cp->host, cp->port, cp->xid, cp->n);
	return 1;
}

static int
Iconv(void *oa, Fconv *f)
{
	char buf[16];
	ulong h;

	h = *(ulong*)oa;
	sprint(buf, "%d.%d.%d.%d", 
		(h>>24)&0xff, (h>>16)&0xff,
		(h>>8)&0xff, h&0xff);
	strconv(buf, f);
	return sizeof(ulong);
}
