/*
 * Point-to-point Tunneling Protocol (PPTP)
 * See RFC 2637, pptpd.c
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <thread.h>

int	ack;
int	alarmed;
int	ctlechotime;
int	ctlfd;
int	ctlrcvtime;
int	debug;
int	grefd;
uchar localip[IPaddrlen];
int	localwin;
char	*keyspec;
int	now;
char	*pppnetmntpt;
int	pid;
Channel *pidchan;
int	pppfd;
int	primary;
int	rack;
Channel	*rdchan;
int	rdexpect;
int	remid;
uchar remoteip[IPaddrlen];
int	remwin;
int	rseq;
int	seq;
char	tcpdir[40];
Channel *tickchan;
int	topppfd;

int	aread(int, int, void*, int);
int	catchalarm(void*, char*);
void	dumpctlpkt(uchar*);
void	getaddrs(void);
void	*emalloc(long);
void	ewrite(int, void*, int);
void	myfatal(char*, ...);
#pragma varargck argpos myfatal 1
int	pptp(char*);
void	pushppp(int);
void	recordack(int);
int	schedack(int, uchar*, int);
void	waitacks(void);

void
usage(void)
{
	fprint(2, "usage: ip/pptp [-Pd] [-k keyspec] [-x pppnetmntpt] [-w window] server\n");
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	int fd;

	ARGBEGIN{
	case 'P':
		primary = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'w':
		localwin = atoi(EARGF(usage()));
		break;
	case 'x':
		pppnetmntpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	fmtinstall('E', eipfmt);
	fmtinstall('I', eipfmt);

	rfork(RFNOTEG);
	atnotify(catchalarm, 1);
	fd = pptp(argv[0]);
	pushppp(fd);
	exits(nil);
}

int
catchalarm(void *a, char *msg)
{
	USED(a);

	if(strstr(msg, "alarm")){
		alarmed = 1;
		return 1;
	}
	if(debug)
		fprint(2, "note rcved: %s\n", msg);
	return 0;
}

enum {
	Stack	= 8192,

	PptpProto	= 0x0100,

	Magic	= 0x1a2b3c4d,
	Window	= 16,		/* default window size */
	Timeout	= 60,		/* timeout in seconds for control channel */
	Pktsize = 2000,		/* maximum packet size */
	Tick	= 500,		/* tick length in milliseconds */
	Sendtimeout = 4,	/* in ticks */

	Servertimeout = 5*60*1000/Tick,
	Echointerval = 60*1000/Tick,
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

void
recho(uchar *in)
{
	uchar out[20];

	if(nhgets(in) < 16)
		return;

	memset(out, 0, sizeof out);
	hnputs(out, sizeof out);
	hnputs(out+2, 1);
	hnputl(out+4, Magic);
	hnputs(out+8, Recho);
	memmove(out+12, in+12, 4);
	out[16] = 1;

	ewrite(ctlfd, out, sizeof out);
}

void
sendecho(void)
{
	uchar out[16];

	ctlechotime = now;	
	memset(out, 0, sizeof out);
	hnputs(out, sizeof out);
	hnputs(out+2, 1);
	hnputl(out+4, Magic);
	hnputs(out+8, Techo);

	ewrite(ctlfd, out, sizeof out);
}

void
pptpctlproc(void*)
{
	uchar pkt[1600], *p;
	int len;

	for(;;){
		if(readn(ctlfd, pkt, 2) != 2)
			myfatal("pptpread: %r");
		len = nhgets(pkt);
		if(len < 12 || len+2 >= sizeof pkt)
			myfatal("pptpread: bad length %d", len);
		if(readn(ctlfd, pkt+2, len-2) != len-2)
			myfatal("pptpread: %r");
		if(nhgetl(pkt+4) != Magic)
			myfatal("pptpread bad magic");
		if(nhgets(pkt+2) != 1)
			myfatal("pptpread bad message type");
		if(debug)
			dumpctlpkt(pkt);
		ctlrcvtime = now;

		switch(nhgets(pkt+8)){
		case Tstart:
		case Tstop:
		case Tcallout:
		case Tcallreq:
		case Tcallclear:
		case Acallcon:
		case Acalldis:
		case Awaninfo:
			myfatal("unexpected msg type %d", nhgets(pkt+8));
		case Techo:
			recho(pkt);
			break;
		case Recho:
			break;
		case Rstart:
		case Rstop:
		case Rcallout:
		case Rcallreq:
			if(rdexpect != nhgets(pkt+8))
				continue;
			p = emalloc(len);
			memmove(p, pkt, len);
			sendp(rdchan, p);
			break;
		case Alinkinfo:
			myfatal("cannot change ppp params on the fly");
		}
	}
}

enum {
	Seqnum = 0x1000,
	Acknum = 0x0080,

	GrePPP = 0x880B,
};

void
grereadproc(void*)
{
	int datoff, flags, len, n, pass;
	uchar pkt[1600];
	uchar src[IPaddrlen], dst[IPaddrlen];

	rfork(RFFDG);
	close(pppfd);
	sendul(pidchan, getpid());

	while((n = read(grefd, pkt, sizeof pkt)) > 0){
		if(n == sizeof pkt)
			myfatal("gre pkt buffer too small");
		if(n < 16){
			if(debug)
				fprint(2, "small pkt len %d ignored\n", n);
			continue;
		}
		v4tov6(src, pkt);
		v4tov6(dst, pkt+4);
		if(ipcmp(src, remoteip) != 0 || ipcmp(dst, localip) != 0)
			myfatal("%I: gre read bad address src=%I dst=%I",
				remoteip, src, dst);
		if(nhgets(pkt+10) != GrePPP)
			myfatal("%I: gre read bad protocol 0x%x",
				remoteip, nhgets(pkt+10));

		flags = nhgets(pkt+8);
		if((flags&0xEF7F) != 0x2001){
			if(debug)
				fprint(2, "bad flags in gre hdr 0x%x\n", flags);
			continue;
		}
		datoff = 8+8;
		pass = 0;
		len = nhgets(pkt+8+4);
		if(len > n-datoff){
			fprint(2, "bad payload length %d > %d\n",
				len, n-datoff);
			continue;
		}
		if(flags&Seqnum)
			datoff += 4;
		if(flags&Acknum){
			recordack(nhgetl(pkt+datoff));
			datoff += 4;
		}
		if(flags&Seqnum)
			pass = schedack(nhgetl(pkt+8+8), pkt+datoff, len);
		if(debug)
			fprint(2, "got gre callid %d len %d flag 0x%x pass %d seq %d rseq %d\n", nhgets(pkt+8+6),
				len, flags, pass, nhgetl(pkt+8+8), rseq);
	}
	threadexits(nil);
}

void
pppreadproc(void*)
{
	int n, myrseq;
	uchar pkt[1600];
	enum {
		Hdr = 8+16,
	};

	rfork(RFFDG);
	close(pppfd);
	sendul(pidchan, getpid());

	while((n = read(topppfd, pkt+Hdr, sizeof pkt-Hdr)) > 0){
		if(n == sizeof pkt-Hdr)
			myfatal("ppp pkt buffer too small");
		v6tov4(pkt+0, localip);
		v6tov4(pkt+4, remoteip);
		hnputs(pkt+8, 0x2001 | Seqnum | Acknum);
		hnputs(pkt+10, GrePPP);
		hnputs(pkt+12, n);
		hnputs(pkt+14, remid);
		hnputl(pkt+16, ++seq);
		myrseq = rseq;
		hnputl(pkt+20, myrseq);
		rack = myrseq;
		if(debug)
			fprint(2, "wrote gre callid %d len %d flag 0x%x seq %d rseq %d\n", nhgets(pkt+8+6),
				n, nhgets(pkt+8), nhgetl(pkt+16), nhgetl(pkt+20));
		if(write(grefd, pkt, n+Hdr) != n+Hdr)
			myfatal("gre write: %r");
		waitacks();
	}
	threadexits(nil);
}

void
sendack(void)
{
	int myrseq;
	uchar pkt[20];

	v6tov4(pkt+0, localip);
	v6tov4(pkt+4, remoteip);
	hnputs(pkt+8, 0x2001 | Acknum);
	hnputs(pkt+10, GrePPP);
	hnputs(pkt+12, 0);
	hnputs(pkt+14, remid);
	myrseq = rseq;
	rack = myrseq;
	hnputs(pkt+16, myrseq);

	if(write(grefd, pkt, sizeof pkt) != sizeof pkt)
		myfatal("gre write: %r");
}

int
schedack(int n, uchar *dat, int len)
{
	static uchar sdat[1600];
	static int srseq, slen;

	if(n-rseq <= 0){
		fprint(2, "skipping pkt %d len %d, have %d\n", n, len, rseq);
		return 0;
	}

	/* missed one pkt, maybe a swap happened, save pkt */
	if(n==rseq+2){
		memmove(sdat, dat, len);
		slen = len;
		srseq = n;
		return 0;
	}

	if(n-rseq > 1){
		if(slen && srseq == n-1){	
			fprint(2, "reswapped pkts %d and %d\n", srseq, n);
			write(topppfd, sdat, slen);
			slen = 0;
		}else
			fprint(2, "missed pkts %d-%d, got %d len %d\n", rseq+1, n-1, n, len);
	}
	write(topppfd, dat, len);
	rseq = n;

	/* send ack if we haven't recently */
	if((int)(rseq-rack) > (localwin>>1))
		sendack();

	return 1;
}

void
gretimeoutproc(void*)
{
	for(;;){
		sleep(Tick);
		now++;
		nbsendul(tickchan, now);
		if(now - ctlrcvtime > Servertimeout)
			myfatal("server timeout");
		if(now - ctlechotime > Echointerval)
			sendecho();
	}
}

void
recordack(int n)
{
	ack = n;
}

void
waitacks(void)
{
/*
	int start;

	start = now;
	while(seq-ack > remwin && now-start < Sendtimeout){
		print("seq %d ack %d remwin %d now %d start %d\n",
			seq, ack, remwin, now, start);
		recvul(tickchan);
	}
*/
}

void
tstart(void)
{
	char *name;
	uchar pkt[200], *rpkt;

	memset(pkt, 0, sizeof pkt);

	hnputs(pkt+0, 156);
	hnputs(pkt+2, 1);
	hnputl(pkt+4, Magic);
	hnputs(pkt+8, Tstart);
	hnputs(pkt+12, PptpProto);
	hnputl(pkt+16, 1);
	hnputl(pkt+20, 1);
	hnputs(pkt+24, 1);
	name = sysname();
	if(name == nil)
		name = "gnot";
	strcpy((char*)pkt+28, name);
	strcpy((char*)pkt+92, "plan 9");

	if(debug)
		dumpctlpkt(pkt);

	rdexpect = Rstart;
	ewrite(ctlfd, pkt, 156);

	rpkt = recvp(rdchan);
	if(rpkt == nil)
		myfatal("recvp: %r");
	if(nhgets(rpkt) != 156)
		myfatal("Rstart wrong length %d != 156", nhgets(rpkt));
	if(rpkt[14] != 1)
		myfatal("Rstart error %d", rpkt[15]);
	free(rpkt);
}

void
tcallout(void)
{
	uchar pkt[200], *rpkt;

	pid = getpid();

	memset(pkt, 0, sizeof pkt);
	hnputs(pkt+0, 168);
	hnputs(pkt+2, 1);
	hnputl(pkt+4, Magic);
	hnputs(pkt+8, Tcallout);

	hnputl(pkt+16, 56000);
	hnputl(pkt+20, 768000);
	hnputl(pkt+24, 3);
	hnputl(pkt+28, 3);
	if(localwin == 0)
		localwin = Window;
	hnputs(pkt+32, localwin);

	if(debug)
		dumpctlpkt(pkt);

	rdexpect = Rcallout;
	ewrite(ctlfd, pkt, 168);

	rpkt = recvp(rdchan);
	if(rpkt == nil)
		myfatal("recvp: %r");
	if(nhgets(rpkt) != 32)
		myfatal("Rcallreq wrong length %d != 32", nhgets(rpkt));
	if(rpkt[16] != 1)
		myfatal("Rcallreq error %d", rpkt[17]);
	remid = nhgets(pkt+12);
	remwin = nhgets(pkt+24);
	free(rpkt);
}

/*
void
tcallreq(void)
{
	uchar pkt[200], *rpkt;

	pid = getpid();

	memset(pkt, 0, sizeof pkt);
	hnputs(pkt+0, 220);
	hnputs(pkt+2, 1);
	hnputl(pkt+4, Magic);
	hnputs(pkt+8, Tcallreq);

	if(debug)
		dumpctlpkt(pkt);

	rdexpect = Rcallreq;
	ewrite(ctlfd, pkt, 220);

	rpkt = recvp(rdchan);
	if(rpkt == nil)
		myfatal("recvp: %r");
	if(nhgets(rpkt) != 24)
		myfatal("Rcallreq wrong length %d != 24", nhgets(rpkt));
	if(rpkt[16] != 1)
		myfatal("Rcallreq error %d", rpkt[17]);
	remid = nhgets(pkt+12);
	remwin = nhgets(pkt+18);
	free(rpkt);
}

void
acallcon(void)
{
	uchar pkt[200];

	memset(pkt, 0, sizeof pkt);
	hnputs(pkt+0, 28);
	hnputs(pkt+2, 1);
	hnputl(pkt+4, Magic);
	hnputs(pkt+8, Acallcon);
	hnputs(pkt+12, remid);
	if(localwin == 0)
		localwin = Window;
	hnputs(pkt+20, localwin);
	hnputl(pkt+24, 1);

	if(debug)
		dumpctlpkt(pkt);

	ewrite(ctlfd, pkt, 28);
}
*/

int
pptp(char *addr)
{
	int p[2];
	char greaddr[128];

	addr = netmkaddr(addr, "net", "pptp");
	ctlfd = dial(addr, nil, tcpdir, nil);
	if(ctlfd < 0)
		myfatal("dial %s: %r", addr);
 	getaddrs();

	rdchan = chancreate(sizeof(void*), 0);
	proccreate(pptpctlproc, nil, Stack);

	tstart();
	tcallout();

	if(pipe(p) < 0)
		myfatal("pipe: %r");

	pppfd = p[0];
	topppfd = p[1];

	strcpy(greaddr, tcpdir);
	*strrchr(greaddr, '/') = '\0';
	sprint(strrchr(greaddr, '/')+1, "gre!%I!%d", remoteip, GrePPP);

	print("local %I remote %I gre %s remid %d remwin %d\n",
		localip, remoteip, greaddr, remid, remwin);

	grefd = dial(greaddr, nil, nil, nil);
	if(grefd < 0)
		myfatal("dial gre: %r");

	tickchan = chancreate(sizeof(int), 0);
	proccreate(gretimeoutproc, nil, Stack);

	pidchan = chancreate(sizeof(int), 0);
	proccreate(grereadproc, nil, Stack);
	recvul(pidchan);
	proccreate(pppreadproc, nil, Stack);
	recvul(pidchan);

	close(topppfd);
	return pppfd;
}
	
void
pushppp(int fd)
{
	char *argv[16];
	int argc;

	argc = 0;
	argv[argc++] = "/bin/ip/ppp";
	argv[argc++] = "-C";
	argv[argc++] = "-m1450";
	if(debug)
		argv[argc++] = "-d";
	if(primary)
		argv[argc++] = "-P";
	if(pppnetmntpt){
		argv[argc++] = "-x";
		argv[argc++] = pppnetmntpt;
	}
	if(keyspec){
		argv[argc++] = "-k";
		argv[argc++] = keyspec;
	}
	argv[argc] = nil;

	switch(fork()){
	case -1:
		myfatal("fork: %r");
	default:
		return;
	case 0:
		dup(fd, 0);
		dup(fd, 1);
		exec(argv[0], argv);
		myfatal("exec: %r");
	}
}

int
aread(int timeout, int fd, void *buf, int nbuf)
{
	int n;

	alarmed = 0;
	alarm(timeout);
	n = read(fd, buf, nbuf);
	alarm(0);
	if(alarmed)
		return -1;
	if(n < 0)
		myfatal("read: %r");
	if(n == 0)
		myfatal("short read");
	return n;
}

void
ewrite(int fd, void *buf, int nbuf)
{
	char e[ERRMAX], path[64];

	if(write(fd, buf, nbuf) != nbuf){
		rerrstr(e, sizeof e);
		strcpy(path, "unknown");
		fd2path(fd, path, sizeof path);
		myfatal("write %d to %s: %s", nbuf, path, e);
	}
}

void*
emalloc(long n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		myfatal("out of memory");
	return v;
}

int
thread(void(*f)(void*), void *a)
{
	int pid;
	pid=rfork(RFNOWAIT|RFMEM|RFPROC);
	if(pid < 0)
		myfatal("rfork: %r");
	if(pid != 0)
		return pid;
	(*f)(a);
	_exits(nil);
	return 0; // never reaches here
}

void
dumpctlpkt(uchar *pkt)
{
	fprint(2, "pkt len %d mtype %d cookie 0x%.8ux type %d\n",
		nhgets(pkt), nhgets(pkt+2),
		nhgetl(pkt+4), nhgets(pkt+8));

	switch(nhgets(pkt+8)){
	default:
		fprint(2, "\tunknown type\n");
		break;
	case Tstart:
		fprint(2, "\tTstart proto %d framing %d bearer %d maxchan %d firmware %d\n",
			nhgets(pkt+12), nhgetl(pkt+16),
			nhgetl(pkt+20), nhgets(pkt+24),
			nhgets(pkt+26));
		fprint(2, "\thost %.64s\n", (char*)pkt+28);
		fprint(2, "\tvendor %.64s\n", (char*)pkt+92);
		break;
	case Rstart:
		fprint(2, "\tRstart proto %d res %d err %d framing %d bearer %d maxchan %d firmware %d\n",
			nhgets(pkt+12), pkt[14], pkt[15],
			nhgetl(pkt+16),
			nhgetl(pkt+20), nhgets(pkt+24),
			nhgets(pkt+26));
		fprint(2, "\thost %.64s\n", (char*)pkt+28);
		fprint(2, "\tvendor %.64s\n", (char*)pkt+92);
		break;

	case Tstop:
		fprint(2, "\tTstop reason %d\n", pkt[12]);
		break;

	case Rstop:
		fprint(2, "\tRstop res %d err %d\n", pkt[12], pkt[13]);
		break;

	case Techo:
		fprint(2, "\tTecho id %.8ux\n", nhgetl(pkt+12));
		break;

	case Recho:
		fprint(2, "\tRecho id %.8ux res %d err %d\n", nhgetl(pkt+12), pkt[16], pkt[17]);
		break;

	case Tcallout:
		fprint(2, "\tTcallout id %d serno %d bps %d-%d\n",
			nhgets(pkt+12), nhgets(pkt+14),
			nhgetl(pkt+16), nhgetl(pkt+20));
		fprint(2, "\tbearer 0x%x framing 0x%x recvwin %d delay %d\n",
			nhgetl(pkt+24), nhgetl(pkt+28),
			nhgets(pkt+32), nhgets(pkt+34));
		fprint(2, "\tphone len %d num %.64s\n", 
			nhgets(pkt+36), (char*)pkt+40);
		fprint(2, "\tsubaddr %.64s\n", (char*)pkt+104);
		break;

	case Rcallout:
		fprint(2, "\tRcallout id %d peerid %d res %d err %d cause %d\n",
			nhgets(pkt+12), nhgets(pkt+14),
			pkt[16], pkt[17], nhgets(pkt+18));
		fprint(2, "\tconnect %d recvwin %d delay %d chan 0x%.8ux\n",
			nhgetl(pkt+20), nhgets(pkt+24),
			nhgets(pkt+26), nhgetl(pkt+28));
		break;

	case Tcallreq:
		fprint(2, "\tTcallreq id %d serno %d bearer 0x%x id 0x%x\n",
			nhgets(pkt+12), nhgets(pkt+14),
			nhgetl(pkt+16), nhgetl(pkt+20));
		fprint(2, "\tdialed len %d num %.64s\n",
			nhgets(pkt+24), (char*)pkt+28);
		fprint(2, "\tdialing len %d num %.64s\n",
			nhgets(pkt+26), (char*)pkt+92);
		fprint(2, "\tsubaddr %.64s\n", (char*)pkt+156);
		break;

	case Rcallreq:
		fprint(2, "\tRcallout id %d peerid %d res %d err %d recvwin %d delay %d\n",
			nhgets(pkt+12), nhgets(pkt+14),
			pkt[16], pkt[17], nhgets(pkt+18),
			nhgets(pkt+20));
		break;

	case Acallcon:
		fprint(2, "\tAcallcon peerid %d connect %d recvwin %d delay %d framing 0x%x\n",
			nhgets(pkt+12), nhgetl(pkt+16),
			nhgets(pkt+20), nhgets(pkt+22),
			nhgetl(pkt+24));
		break;

	case Tcallclear:
		fprint(2, "\tTcallclear callid %d\n",
			nhgets(pkt+12));
		break;

	case Acalldis:
		fprint(2, "\tAcalldis callid %d res %d err %d cause %d\n",
			nhgets(pkt+12), pkt[14], pkt[15],
			nhgets(pkt+16));
		fprint(2, "\tstats %.128s\n", (char*)pkt+20);
		break;

	case Awaninfo:
		fprint(2, "\tAwaninfo peerid %d\n", nhgets(pkt+12));
		fprint(2, "\tcrc errors %d\n", nhgetl(pkt+16));
		fprint(2, "\tframe errors %d\n", nhgetl(pkt+20));
		fprint(2, "\thardware overruns %d\n", nhgetl(pkt+24));
		fprint(2, "\tbuffer overruns %d\n", nhgetl(pkt+28));
		fprint(2, "\ttime-out errors %d\n", nhgetl(pkt+32));
		fprint(2, "\talignment errors %d\n", nhgetl(pkt+36));
		break;

	case Alinkinfo:
		fprint(2, "\tAlinkinfo peerid %d sendaccm 0x%ux recvaccm 0x%ux\n",
			nhgets(pkt+12), nhgetl(pkt+16),
			nhgetl(pkt+20));
		break;
	}
}

void
getaddrs(void)
{
	char buf[128];
	int fd, n;

	sprint(buf, "%s/local", tcpdir);
	if((fd = open(buf, OREAD)) < 0)
		myfatal("could not open %s: %r", buf);
	if((n = read(fd, buf, sizeof(buf))) < 0)
		myfatal("could not read %s: %r", buf);
	buf[n] = 0;
	parseip(localip, buf);
	close(fd);

	sprint(buf, "%s/remote", tcpdir);
	if((fd = open(buf, OREAD)) < 0)
		myfatal("could not open %s: %r", buf);
	if((n = read(fd, buf, sizeof(buf))) < 0)
		myfatal("could not read %s: %r", buf);
	buf[n] = 0;
	parseip(remoteip, buf);
	close(fd);
}

void
myfatal(char *fmt, ...)
{
	char sbuf[512];
	va_list arg;
	uchar buf[16];

	memset(buf, 0, sizeof(buf));
	hnputs(buf+0, sizeof(buf));	/* length */
	hnputs(buf+2, 1);		/* message type */
	hnputl(buf+4, Magic);		/* magic */
	hnputs(buf+8, Tstop);		/* op */
	buf[12] = 3;			/* local shutdown */
	write(ctlfd, buf, sizeof(buf));

	va_start(arg, fmt);
	vseprint(sbuf, sbuf+sizeof(sbuf), fmt, arg);
	va_end(arg);

	fprint(2, "fatal: %s\n", sbuf);
	threadexitsall(nil);
}
