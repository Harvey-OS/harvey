#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>

#define	LOG	"pptpd"

typedef struct Call	Call;
typedef struct Event Event;

#define	SDB	if(debug) fprint(2, 
#define EDB	);

enum {
	Magic	= 0x1a2b3c4d,
	Nhash	= 17,
	Nchan	= 10,		/* maximum number of channels */
	Window	= 8,		/* default window size */
	Timeout	= 60,		/* timeout in seconds for control channel */
	Pktsize = 2000,		/* maximum packet size */
	Tick	= 500,		/* tick length in milliseconds */
	Sendtimeout = 4,	/* in ticks */
};

enum {
	Syncframe	= 0x1,
	Asyncframe	= 0x2,
	Analog		= 0x1,
	Digital		= 0x2,
	Version		= 0x100,
};

enum {
	Tstart		= 1,
	Rstart		= 2,
	Tstop		= 3,
	Rstop		= 4,
	Techo		= 5,
	Recho		= 6,
	Tcallout	= 7,
	Rcallout	= 8,
	Tcallreq	= 9,
	Rcallreq	= 10,
	Acallcon	= 11,
	Tcallclear	= 12,
	Acalldis	= 13,
	Awaninfo	= 14,
	Alinkinfo	= 15,
};


struct Event {
	QLock;
	QLock waitlk;
	int	wait;
	int ready;
};

struct Call {
	int	ref;
	QLock	lk;
	int	id;
	int	serial;
	int	pppfd;

	int	closed;

	int	pac;	/* server is acting as a PAC */

	int	recvwindow;	/* recv windows */
	int	sendwindow;	/* send windows */
	int	delay;

	int	sendaccm;
	int	recvaccm;

	uint	seq;		/* current seq number - for send */
	uint	ack;		/* current acked mesg - for send */
	uint	rseq;		/* highest recv seq number for in order packet  */
	uint	rack;		/* highest ack sent */

	Event	eack;		/* recved ack - for send */
	ulong	tick;

	uchar	remoteip[IPaddrlen];	/* remote ip address */
	int	dhcpfd[2];	/* pipe to dhcpclient */

	/* error stats */
	struct {
		int	crc;
		int	frame;
		int	hardware;
		int	overrun;
		int	timeout;
		int	align;
	} err;

	struct {
		int	send;
		int	sendack;
		int	recv;
		int	recvack;
		int	dropped;
		int	missing;
		int	sendwait;
		int	sendtimeout;
	} stat;

	Call	*next;
};

struct {
	QLock	lk;
	int	start;
	int	grefd;
	int	grecfd;
	uchar	local[IPaddrlen];
	uchar	remote[IPaddrlen];
	char	*tcpdir;
	uchar	ipaddr[IPaddrlen];		/* starting ip addresss to allocate */

	int	recvwindow;

	char	*pppdir;
	char	*pppexec;

	double	rcvtime;	/* time at which last request was received */
	int	echoid;		/* id of last echo request */

	Call	*hash[Nhash];
} srv;

/* GRE flag bits */
enum {
	GRE_chksum	= (1<<15),
	GRE_routing	= (1<<14),
	GRE_key		= (1<<13),
	GRE_seq		= (1<<12),
	GRE_srcrt	= (1<<11),
	GRE_recur	= (7<<8),
	GRE_ack		= (1<<7),
	GRE_ver		= 0x7,
};

/* GRE protocols */
enum {
	GRE_ppp		= 0x880b,
};

int	debug;
double	drop;

void	myfatal(char *fmt, ...);

#define	PSHORT(p, v)		((p)[0]=((v)>>8), (p)[1]=(v))
#define	PLONG(p, v)		(PSHORT(p, (v)>>16), PSHORT(p+2, (v)))
#define	PSTRING(d,s,n)		strncpy((char*)(d), s, n)
#define	GSHORT(p)		(((p)[0]<<8) | ((p)[1]<<0))
#define	GLONG(p)		((GSHORT((p))<<16) | ((GSHORT((p)+2))<<0))
#define	GSTRING(d,s,n)		strncpy(d, (char*)(s), n), d[(n)-1] = 0

void	serve(void);

int	sstart(uchar*, int);
int	sstop(uchar*, int);
int	secho(uchar*, int);
int	scallout(uchar*, int);
int	scallreq(uchar*, int);
int	scallcon(uchar*, int);
int	scallclear(uchar*, int);
int	scalldis(uchar*, int);
int	swaninfo(uchar*, int);
int	slinkinfo(uchar*, int);

Call	*callalloc(int id);
void	callclose(Call*);
void	callfree(Call*);
Call	*calllookup(int id);

void	gretimeout(void*);
void	pppread(void*);

void	srvinit(void);
void	greinit(void);
void	greread(void*);
void	greack(Call *c);

void	timeoutthread(void*);

int	argatoi(char *p);
void	usage(void);
int	ipaddralloc(Call *c);

void	*emallocz(int size);
void	esignal(Event *e);
void	ewait(Event *e);
int	proc(char **argv, int fd0, int fd1, int fd2);
double	realtime(void);
ulong	thread(void(*f)(void*), void *a);

void
main(int argc, char *argv[])
{
	ARGBEGIN{
	case 'd': debug++; break;
	case 'p': srv.pppdir = ARGF(); break;
	case 'P': srv.pppexec = ARGF(); break;
	case 'w': srv.recvwindow = argatoi(ARGF()); break;
	case 'D': drop = atof(ARGF()); break;
	default:
		usage();
	}ARGEND

	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M', eipfmt);

	rfork(RFNOTEG|RFREND);

	if(argc != 1)
		usage();

	srv.tcpdir = argv[0];

	srvinit();

	syslog(0, LOG, ": src=%I: pptp started: %d", srv.remote, getpid());

	SDB  "\n\n\n%I: pptp started\n", srv.remote EDB

	greinit();

	thread(timeoutthread, 0);

	serve();

	syslog(0, LOG, ": src=%I: server exits", srv.remote);

	exits(0);
}

void
usage(void)
{
	fprint(2, "usage: pptpd [-dD] [-p ppp-net] [-w window] tcpdir\n");
	exits("usage");
}

void
serve(void)
{
	uchar buf[2000], *p;
	int n, n2, len;
	int magic;
	int op, type;

	n = 0;
	for(;;) {
		n2 = read(0, buf+n, sizeof(buf)-n);
		if(n2 < 0)
			myfatal("bad read on ctl channel: %r");
		if(n2 == 0)
			break;
		n += n2;
		p = buf;
		for(;;) {
			if(n < 12)
				break;

			qlock(&srv.lk);
			srv.rcvtime = realtime();
			qunlock(&srv.lk);

			len = GSHORT(p);
			type = GSHORT(p+2);
			magic = GLONG(p+4);
			op = GSHORT(p+8);
			if(magic != Magic)
				myfatal("bad magic number: got %x", magic);
			if(type != 1)
				myfatal("bad message type: %d", type);
			switch(op) {
			default:
				myfatal("unknown control op: %d", op);
			case Tstart:		/* start-control-connection-request */
				n2 = sstart(p, n);
				break;
			case Tstop:
				n2 = sstop(p, n);
				if(n2 > 0)
					return;
				break;
			case Techo:
				n2 = secho(p, n);
				break;
			case Tcallout:
				n2 = scallout(p, n);
				break;
			case Tcallreq:
				n2 = scallreq(p, n);
				break;
			case Acallcon:
				n2 = scallcon(p, n);
				break;
			case Tcallclear:
				n2 = scallclear(p, n);
				break;
			case Acalldis:
				n2 = scalldis(p, n);
				break;
			case Awaninfo:
				n2 = swaninfo(p, n);
				break;
			case Alinkinfo:
				n2 = slinkinfo(p, n);
				break;
			}	
			if(n2 == 0)
				break;
			if(n2 != len)
				myfatal("op=%d: bad length: got %d expected %d", op, len, n2);
			n -= n2;
			p += n2;
			
		}

		/* move down partial message */
		if(p != buf && n != 0)
			memmove(buf, p, n);
	}

}

int
sstart(uchar *p, int n)
{
	int ver, frame, bearer, maxchan, firm;
	char host[64], vendor[64], *sysname;
	uchar buf[156];

	if(n < 156)
		return 0;
	ver = GSHORT(p+12);
	frame = GLONG(p+16);
	bearer = GLONG(p+20);
	maxchan = GSHORT(p+24);
	firm = GSHORT(p+26);
	GSTRING(host, p+28, 64);
	GSTRING(vendor, p+92, 64);

	SDB "%I: start ver = %x f = %d b = %d maxchan = %d firm = %d host = %s vendor = %s\n",
		srv.remote, ver, frame, bearer, maxchan, firm, host, vendor EDB

	if(ver != Version)
		myfatal("bad version: got %x expected %x", ver, Version);

	if(srv.start)
		myfatal("multiple start messages");

	srv.start = 1;

	sysname = getenv("sysname");
	if(sysname == 0)
		strcpy(host, "gnot");
	else
		strncpy(host, sysname, 64);
	free(sysname);

	memset(buf, 0, sizeof(buf));

	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Rstart);		/* op */
	PSHORT(buf+12, Version);	/* version */
	buf[14] = 1;			/* result = ok */
	PLONG(buf+16, Syncframe|Asyncframe);	/* frameing */
	PLONG(buf+20, Digital|Analog);	/* berear capabilities */
	PSHORT(buf+24, Nchan);		/* max channels */
	PSHORT(buf+26, 1);		/* driver version */
	PSTRING(buf+28, host, 64);	/* host name */
	PSTRING(buf+92, "plan 9", 64);	/* vendor */

	if(write(1, buf, sizeof(buf)) < sizeof(buf))
		myfatal("write failed: %r");

	return 156;
}

int
sstop(uchar *p, int n)
{
	int reason;
	uchar buf[16];

	if(n < 16)
		return 0;
	reason = p[12];
	
	SDB "%I: stop %d\n", srv.remote, reason EDB

	memset(buf, 0, sizeof(buf));
	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Rstop);		/* op */
	buf[12] = 1;			/* ok */

	if(write(1, buf, sizeof(buf)) < sizeof(buf))
		myfatal("write failed: %r");

	return 16;
}

int
secho(uchar *p, int n)
{
	int id;
	uchar buf[20];

	if(n < 16)
		return 0;
	id = GLONG(p+12);
	
	SDB "%I: echo %d\n", srv.remote, id EDB

	memset(buf, 0, sizeof(buf));
	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Recho);		/* op */
	PLONG(buf+12, id);		/* id */
	p[16] = 1;			/* ok */

	if(write(1, buf, sizeof(buf)) < sizeof(buf))
		myfatal("write failed: %r");

	return 16;
}

int
scallout(uchar *p, int n)
{
	int id, serial;
	int minbps, maxbps, bearer, frame;
	int window, delay;
	int nphone;
	char phone[64], sub[64], buf[32];
	Call *c;

	if(n < 168)
		return 0;

	if(!srv.start)
		myfatal("%I: did not recieve start message", srv.remote);
	
	id = GSHORT(p+12);
	serial = GSHORT(p+14);
	minbps = GLONG(p+16);
	maxbps = GLONG(p+20);
	bearer = GLONG(p+24);
	frame = GLONG(p+28);
	window = GSHORT(p+32);
	delay = GSHORT(p+34);
	nphone = GSHORT(p+36);
	GSTRING(phone, p+40, 64);
	GSTRING(sub, p+104, 64);

	SDB "%I: callout id = %d serial = %d bps=[%d,%d] b=%x f=%x win = %d delay = %d np=%d phone=%s sub=%s\n",
		srv.remote, id, serial, minbps, maxbps, bearer, frame, window, delay, nphone, phone, sub EDB

	c = callalloc(id);
	c->sendwindow = window;
	c->delay = delay;
	c->pac = 1;
	c->recvwindow = srv.recvwindow;

	memset(buf, 0, sizeof(buf));
	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Rcallout);	/* op */
	PSHORT(buf+12, id);		/* call id */
	PSHORT(buf+14, id);		/* peer id */
	buf[16] = 1;			/* ok */
	PLONG(buf+20, 10000000);	/* speed */
	PSHORT(buf+24, c->recvwindow);	/* window size */
	PSHORT(buf+26, 0);		/* delay */
	PLONG(buf+28, 0);		/* channel id */
	
	if(write(1, buf, sizeof(buf)) < sizeof(buf))
		myfatal("write failed: %r");

	return 168;
}

int
scallreq(uchar *p, int n)
{
	USED(p);
	USED(n);

	myfatal("callreq: not done yet");
	return 0;
}

int
scallcon(uchar *p, int n)
{
	USED(p);
	USED(n);

	myfatal("callcon: not done yet");
	return 0;
}

int
scallclear(uchar *p, int n)
{
	Call *c;
	int id;
	uchar buf[148];

	if(n < 16)
		return 0;
	id = GSHORT(p+12);
	
	SDB "%I: callclear id=%d\n", srv.remote, id EDB
	
	if(c = calllookup(id)) {
		callclose(c);
		callfree(c);
	}

	memset(buf, 0, sizeof(buf));
	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Acalldis);	/* op */
	PSHORT(buf+12, id);		/* id */
	buf[14] = 3;			/* reply to callclear */

	if(write(1, buf, sizeof(buf)) < sizeof(buf))
		myfatal("write failed: %r");

	return 16;
}

int
scalldis(uchar *p, int n)
{
	Call *c;
	int id, res;

	if(n < 148)
		return 0;
	id = GSHORT(p+12);
	res = p[14];

	SDB "%I: calldis id=%d res=%d\n", srv.remote, id, res EDB

	if(c = calllookup(id)) {
		callclose(c);
		callfree(c);
	}

	return 148;
}

int
swaninfo(uchar *p, int n)
{
	Call *c;
	int id;

	if(n < 40)
		return 0;
	
	id = GSHORT(p+12);
	SDB "%I: waninfo id = %d\n", srv.remote, id EDB
	
	c = calllookup(id);
	if(c != 0) {
		c->err.crc = GLONG(p+16);
		c->err.frame = GLONG(p+20);
		c->err.hardware = GLONG(p+24);
		c->err.overrun = GLONG(p+28);
		c->err.timeout = GLONG(p+32);
		c->err.align = GLONG(p+36);

		callfree(c);
	}

	
	return 40;
}

int
slinkinfo(uchar *p, int n)
{
	Call *c;
	int id;
	int sendaccm, recvaccm;

	if(n < 24)
		return 0;
	id = GSHORT(p+12);
	sendaccm = GLONG(p+16);
	recvaccm = GLONG(p+20);

	SDB "%I: linkinfo id=%d saccm=%ux raccm=%ux\n", srv.remote, id, sendaccm, recvaccm EDB

	if(c = calllookup(id)) {
		c->sendaccm = sendaccm;
		c->recvaccm = recvaccm;

		callfree(c);
	}
	
	return 24;
}

Call*
callalloc(int id)
{
	uint h;
	Call *c;
	char buf[300], *argv[30], local[20], remote[20], **p;
	int fd, pfd[2], n;

	h = id%Nhash;

	qlock(&srv.lk);
	for(c=srv.hash[h]; c; c=c->next)
		if(c->id == id)
			myfatal("callalloc: duplicate id: %d", id);
	c = emallocz(sizeof(Call));
	c->ref = 1;
	c->id = id;
	c->sendaccm = ~0;
	c->recvaccm = ~0;

	if(!ipaddralloc(c))
		myfatal("callalloc: could not alloc remote ip address");

	if(pipe(pfd) < 0)
		myfatal("callalloc: pipe failed: %r");

	sprint(buf, "%s/ipifc/clone", srv.pppdir);
	fd = open(buf, OWRITE);
	if(fd < 0)
		myfatal("callalloc: could not open %s: %r", buf);

	n = sprint(buf, "iprouting");
	if(write(fd, buf, n) < n)
		myfatal("callalloc: write to ifc failed: %r");
	close(fd);

	p = argv;
	*p++ = srv.pppexec;
	*p++ = "-SC";
	*p++ = "-x";
	*p++ = srv.pppdir;
	if(debug)
		*p++ = "-d";
	sprint(local, "%I", srv.ipaddr);
	*p++ = local;
	sprint(remote, "%I", c->remoteip);
	*p++ = remote;
	*p = 0;

	proc(argv, pfd[0], pfd[0], 2);

	close(pfd[0]);

	c->pppfd = pfd[1];

	c->next = srv.hash[h];
	srv.hash[h] = c;

	qunlock(&srv.lk);

	c->ref++;
	thread(pppread, c);
	c->ref++;
	thread(gretimeout, c);

	syslog(0, LOG, ": src=%I: call started: id=%d: remote ip=%I", srv.remote, id, c->remoteip);

	return c;
}

void
callclose(Call *c)
{
	Call *oc;
	int id;
	uint h;

	syslog(0, LOG, ": src=%I: call closed: id=%d: send=%d sendack=%d recv=%d recvack=%d dropped=%d missing=%d sendwait=%d sendtimeout=%d",
		srv.remote, c->id, c->stat.send, c->stat.sendack, c->stat.recv, c->stat.recvack,
		c->stat.dropped, c->stat.missing, c->stat.sendwait, c->stat.sendtimeout);

	qlock(&srv.lk);
	if(c->closed) {
		qunlock(&srv.lk);
		return;
	}
	c->closed = 1;

	close(c->dhcpfd[0]);
	close(c->dhcpfd[1]);
	close(c->pppfd);
	c->pppfd = -1;

	h = c->id%Nhash;
	id = c->id;
	for(c=srv.hash[h],oc=0; c; oc=c,c=c->next)
		if(c->id == id)
			break;
	if(oc == 0)
		srv.hash[h] = c->next;
	else
		oc->next = c->next;
	c->next = 0;
	qunlock(&srv.lk);

	callfree(c);
}

void
callfree(Call *c)
{	
	int ref;
	
	qlock(&srv.lk);
	ref = --c->ref;
	qunlock(&srv.lk);
	if(ref > 0)
		return;
	
	/* already unhooked from hash list - see callclose */
	assert(c->closed == 1);
	assert(ref == 0);
	assert(c->next == 0);

SDB "call free\n" EDB	
	free(c);
}

Call*
calllookup(int id)
{
	uint h;
	Call *c;

	h = id%Nhash;

	qlock(&srv.lk);
	for(c=srv.hash[h]; c; c=c->next)
		if(c->id == id)
			break;
	if(c != 0)
		c->ref++;
	qunlock(&srv.lk);

	return c;
}


void
srvinit(void)
{
	char buf[100];
	int fd, n;

	sprint(buf, "%s/local", srv.tcpdir);
	if((fd = open(buf, OREAD)) < 0)
		myfatal("could not open %s: %r", buf);
	if((n = read(fd, buf, sizeof(buf))) < 0)
		myfatal("could not read %s: %r", buf);
	buf[n] = 0;
	parseip(srv.local, buf);
	close(fd);

	sprint(buf, "%s/remote", srv.tcpdir);
	if((fd = open(buf, OREAD)) < 0)
		myfatal("could not open %s: %r", buf);
	if((n = read(fd, buf, sizeof(buf))) < 0)
		myfatal("could not read %s: %r", buf);
	buf[n] = 0;
	parseip(srv.remote, buf);
	close(fd);

	if(srv.pppdir == 0)
		srv.pppdir = "/net";

	if(srv.pppexec == 0)
		srv.pppexec = "/bin/ip/ppp";

	if(myipaddr(srv.ipaddr, srv.pppdir) < 0)
		myfatal("could not read local ip addr: %r");
	if(srv.recvwindow == 0)
		srv.recvwindow = Window;
}

void
greinit(void)
{
	char addr[100], *p;
	int fd, cfd;

	SDB "srv.tcpdir = %s\n", srv.tcpdir EDB
	strcpy(addr, srv.tcpdir);
	p = strrchr(addr, '/');
	if(p == 0)
		myfatal("bad tcp dir: %s", srv.tcpdir);
	*p = 0;
	p = strrchr(addr, '/');
	if(p == 0)
		myfatal("bad tcp dir: %s", srv.tcpdir);
	sprint(p, "/gre!%I!34827", srv.remote);

	SDB "addr = %s\n", addr EDB

	fd = dial(addr, 0, 0, &cfd);

	if(fd < 0)
		myfatal("%I: dial %s failed: %r", srv.remote, addr);

	srv.grefd = fd;
	srv.grecfd = cfd;

	thread(greread, 0);
}

void
greread(void *)
{
	uchar buf[Pktsize], *p;
	int n, i;
	int flag, prot, len, callid;
	uchar src[IPaddrlen], dst[IPaddrlen];
	uint rseq, ack;
	Call *c;
	static double t, last;

	for(;;) {
		n = read(srv.grefd, buf, sizeof(buf));
		if(n < 0)
			myfatal("%I: bad read on gre: %r", srv.remote);
		if(n == sizeof(buf))
			myfatal("%I: gre read: buf too small", srv.remote);

		p = buf;
		v4tov6(src, p);
		v4tov6(dst, p+4);
		flag = GSHORT(p+8);
		prot = GSHORT(p+10);
		p += 12; n -= 12;

		if(ipcmp(src, srv.remote) != 0 || ipcmp(dst, srv.local) != 0)
			myfatal("%I: gre read bad address src=%I dst=%I", srv.remote, src, dst);

		if(prot != GRE_ppp)
			myfatal("%I: gre read gave bad protocol", srv.remote);

		if(flag & (GRE_chksum|GRE_routing)){
			p += 4; n -= 4;
		}

		if(!(flag&GRE_key))
			myfatal("%I: gre packet does not contain a key: f=%ux",
				srv.remote, flag);

		len = GSHORT(p);
		callid = GSHORT(p+2);
		p += 4; n -= 4;

		c = calllookup(callid);
		if(c == 0) {
			SDB "%I: unknown callid: %d\n", srv.remote, callid EDB
			continue;
		}

		qlock(&c->lk);

		c->stat.recv++;

		if(flag&GRE_seq) {
			rseq = GLONG(p);
			p += 4; n -= 4;
		} else
			rseq = c->rseq;

		if(flag&GRE_ack){
			ack = GLONG(p);
			p += 4; n -= 4;
		} else
			ack = c->ack;

		/* skip routing if present */
		if(flag&GRE_routing) {
			while((i=p[3]) != 0) {
				n -= i;
				p += i;
			}
		}

		if(len > n)
			myfatal("%I: bad len in gre packet", srv.remote);

		if((int)(ack-c->ack) > 0) {
			c->ack = ack;
			esignal(&c->eack);
		}

		if(debug)
			t = realtime();

		if(len == 0) {
			/* ack packet */
			c->stat.recvack++;

SDB "%I: %.3f (%.3f): gre %d: recv ack a=%ux n=%d flag=%ux\n", srv.remote, t, t-last,
	c->id, ack, n, flag EDB

		} else {

SDB "%I: %.3f (%.3f): gre %d: recv s=%ux a=%ux len=%d\n", srv.remote, t, t-last,
	c->id, rseq, ack, len EDB

			/*
			 * the following handles the case of a single pair of packets
			 * received out of order
			 */
			n = rseq-c->rseq;
			if(n > 0 && (drop == 0. || frand() > drop)) {
				c->stat.missing += n-1;
				/* current packet */
				write(c->pppfd, p, len);
			} else {
				/* out of sequence - drop on the floor */
				c->stat.dropped++;

SDB "%I: %.3f: gre %d: recv out of order or dup packet: seq=%ux len=%d\n",
srv.remote, realtime(), c->id, rseq, len EDB

			}
		}

		if((int)(rseq-c->rseq) > 0)
			c->rseq = rseq;

		if(debug)
			last=t;

		/* open up client window */
		if((int)(c->rseq-c->rack) > (c->recvwindow>>1))
			greack(c);

		qunlock(&c->lk);


		callfree(c);
	}
}

void
greack(Call *c)
{
	uchar buf[20];

	c->stat.sendack++;

SDB "%I: %.3f: gre %d: send ack %ux\n", srv.remote, realtime(), c->id, c->rseq EDB

	v6tov4(buf+0, srv.local);		/* source */
	v6tov4(buf+4, srv.remote);		/* source */
	PSHORT(buf+8, GRE_key|GRE_ack|1);
	PSHORT(buf+10, GRE_ppp);
	PSHORT(buf+12, 0);
	PSHORT(buf+14, c->id);
	PLONG(buf+16, c->rseq);

	write(srv.grefd, buf, sizeof(buf));

	c->rack = c->rseq;

}

void
gretimeout(void *a)
{
	Call *c;

	c = a;

	while(!c->closed) {
		sleep(Tick);
		qlock(&c->lk);
		c->tick++;
		qunlock(&c->lk);
		esignal(&c->eack);
	}
	callfree(c);
	exits(0);
}


void
pppread(void *a)
{
	Call *c;
	uchar buf[2000], *p;
	int n;
	ulong tick;

	c = a;
	for(;;) {
		p = buf+24;
		n = read(c->pppfd, p, sizeof(buf)-24);
		if(n <= 0)
			break;
		
		qlock(&c->lk);

		/* add gre header */
		c->seq++;
		tick = c->tick;

		while(c->seq-c->ack>c->sendwindow && c->tick-tick<Sendtimeout && !c->closed) {
			c->stat.sendwait++;
SDB "window full seq = %d ack = %ux window = %ux\n", c->seq, c->ack, c->sendwindow EDB
			qunlock(&c->lk);
			ewait(&c->eack);
			qlock(&c->lk);
		}
		
		if(c->tick-tick >= Sendtimeout) {
			c->stat.sendtimeout++;
SDB "send timeout = %d ack = %ux window = %ux\n", c->seq, c->ack, c->sendwindow EDB
		}

		v6tov4(buf+0, srv.local);		/* source */
		v6tov4(buf+4, srv.remote);		/* source */
		PSHORT(buf+8, GRE_key|GRE_seq|GRE_ack|1);
		PSHORT(buf+10, GRE_ppp);
		PSHORT(buf+12, n);
		PSHORT(buf+14, c->id);
		PLONG(buf+16, c->seq);
		PLONG(buf+20, c->rseq);

		c->stat.send++;
		c->rack = c->rseq;

SDB "%I: %.3f: gre %d: send s=%ux a=%ux len=%d\n", srv.remote, realtime(),
	c->id,  c->seq, c->rseq, n EDB

		if(drop == 0. || frand() > drop)
			if(write(srv.grefd, buf, n+24)<n+24)
				myfatal("pppread: write failed: %r");

		qunlock(&c->lk);
	}

	SDB "pppread exit: %d\n", c->id);
	
	callfree(c);
	exits(0);
}

void
timeoutthread(void*)
{
	for(;;) {
		sleep(30*1000);

		qlock(&srv.lk);
		if(realtime() - srv.rcvtime > 5*60)
			myfatal("server timedout");
		qunlock(&srv.lk);
	}
}


/* use syslog() rather than fprint(2, ...) */
void
myfatal(char *fmt, ...)
{
	char sbuf[512];
	va_list arg;
	uchar buf[16];

	/* NT don't seem to like us just going away */
	memset(buf, 0, sizeof(buf));
	PSHORT(buf+0, sizeof(buf));	/* length */
	PSHORT(buf+2, 1);		/* message type */
	PLONG(buf+4, Magic);		/* magic */
	PSHORT(buf+8, Tstop);		/* op */
	buf[12] = 3;			/* local shutdown */

	write(1, buf, sizeof(buf));

	va_start(arg, fmt);
	vseprint(sbuf, sbuf+sizeof(sbuf), fmt, arg);
	va_end(arg);

	SDB "%I: fatal: %s\n", srv.remote, sbuf EDB
	syslog(0, LOG, ": src=%I: fatal: %s", srv.remote, sbuf);

	close(0);
	close(1);
	close(srv.grefd);
	close(srv.grecfd);

	postnote(PNGROUP, getpid(), "die");
	exits(sbuf);
}

int
argatoi(char *p)
{
	char *q;
	int i;

	if(p == 0)
		usage();

	i = strtol(p, &q, 0);
	if(q == p)
		usage();
	return i;
}

void
dhcpclientwatch(void *a)
{
	Call *c = a;
	uchar buf[1];

	for(;;) {
		if(read(c->dhcpfd[0], buf, sizeof(buf)) <= 0)
			break;
	}
	if(!c->closed)
		myfatal("dhcpclient terminated");
	callfree(c);
	exits(0);
}

int
ipaddralloc(Call *c)
{
	int pfd[2][2];
	char *argv[4], *p;
	Biobuf bio;

	argv[0] = "/bin/ip/dhcpclient";
	argv[1] = "-x";
	argv[2] = srv.pppdir;
	argv[3] = 0;

	if(pipe(pfd[0])<0)
		myfatal("ipaddralloc: pipe failed: %r");
	if(pipe(pfd[1])<0)
		myfatal("ipaddralloc: pipe failed: %r");

	if(proc(argv, pfd[0][0], pfd[1][1], 2) < 0)
		myfatal("ipaddralloc: proc failed: %r");

	close(pfd[0][0]);
	close(pfd[1][1]);
	c->dhcpfd[0] = pfd[1][0];
	c->dhcpfd[1] = pfd[0][1];

	Binit(&bio, pfd[1][0], OREAD);
	for(;;) {
		p = Brdline(&bio, '\n');
		if(p == 0)
			break;
		if(strncmp(p, "ip=", 3) == 0) {
			p += 3;
			parseip(c->remoteip, p);
		} else if(strncmp(p, "end\n", 4) == 0)
			break;
	}

	Bterm(&bio);

	c->ref++;

	thread(dhcpclientwatch, c);

	return ipcmp(c->remoteip, IPnoaddr) != 0;
}


void
esignal(Event *e)
{	
	qlock(e);
	if(e->wait == 0) {
		e->ready = 1;
		qunlock(e);
		return;
	}
	assert(e->ready == 0);
	e->wait = 0;
	rendezvous(e, (void*)1);
	qunlock(e);
}

void
ewait(Event *e)
{
	qlock(&e->waitlk);
	qlock(e);
	assert(e->wait == 0);
	if(e->ready) {
		e->ready = 0;
	} else {	
		e->wait = 1;
		qunlock(e);
		rendezvous(e, (void*)2);
		qlock(e);
	}
	qunlock(e);
	qunlock(&e->waitlk);
}

ulong
thread(void(*f)(void*), void *a)
{
	int pid;
	pid=rfork(RFNOWAIT|RFMEM|RFPROC);
	if(pid < 0)
		myfatal("rfork failed: %r");
	if(pid != 0)
		return pid;
	(*f)(a);
	return 0; // never reaches here
}

double
realtime(void)
{
	long times(long*);

	return times(0) / 1000.0;
}

void *
emallocz(int size)
{
	void *p;
	p = malloc(size);
	if(p == 0)
		myfatal("malloc failed: %r");
	memset(p, 0, size);
	return p;
}

static void
fdclose(void)
{
	int fd, n, i;
	Dir *d, *p;

	if((fd = open("#d", OREAD)) < 0)
		return;

	n = dirreadall(fd, &d);
	for(p = d; n > 0; n--, p++) {
		i = atoi(p->name);
		if(i > 2)
			close(i);
	}
	free(d);
}

int
proc(char **argv, int fd0, int fd1, int fd2)
{
	int r, flag;
	char *arg0, file[200];

	arg0 = argv[0];

	strcpy(file, arg0);

	if(access(file, 1) < 0) {
		if(strncmp(arg0, "/", 1)==0
		|| strncmp(arg0, "#", 1)==0
		|| strncmp(arg0, "./", 2)==0
		|| strncmp(arg0, "../", 3)==0)
			return 0;
		sprint(file, "/bin/%s", arg0);
		if(access(file, 1) < 0)
			return 0;
	}

	flag = RFPROC|RFFDG|RFENVG|RFNOWAIT;
	if((r = rfork(flag)) != 0) {
		if(r < 0)
			return 0;
		return r;
	}

	if(fd0 != 0) {
		if(fd1 == 0)
			fd1 = dup(0, -1);
		if(fd2 == 0)
			fd2 = dup(0, -1);
		close(0);
		if(fd0 >= 0)
			dup(fd0, 0);
	}

	if(fd1 != 1) {
		if(fd2 == 1)
			fd2 = dup(1, -1);
		close(1);
		if(fd1 >= 0)
			dup(fd1, 1);
	}

	if(fd2 != 2) {
		close(2);
		if(fd2 >= 0)
			dup(fd2, 2);
	}

	fdclose();

	exec(file, argv);
	myfatal("proc: exec failed: %r");
	return 0;
}

