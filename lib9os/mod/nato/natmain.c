//#pragma lib "libe.a"
//#pragma src "/usr/ehg/e"

#include "nat.h"
#include "block.h"



#pragma	varargck	argpos	Error	1


char* LOG = "nat";
static int dying;	// flag to signal to all threads to shut down
static double time0;	// time origin for expires

enum{
	Buflen=		4096,		// block size
	Defmtu=		1514,		// default that we will ask for
	Minmtu=		128,		// minimum that we will accept
	Maxmtu=		2000,		// maximum that we will accept
};

typedef struct Qualstats{
	ulong	packets;
	ulong	uchars;
	ulong	discards;
} Qualstats;

typedef struct NAT{
	QLock		insidelock;	// lock for insidefd
	char*		insidenet;	// inside ip stack
	int		insidefd;	//   data
	int		insidecfd;	//   control
	Ipaddr		insideaddr;	//   local inside gateway IP addr
	Ipaddr		insideraddr;
	char*		outsidenet;	// outside ip stack
	Ipaddr		outsideaddr;	//   global outside IP addr
	ulong		mtu;		// maximum xmit size
	Qualstats	in;		// inbound
	Qualstats	out;		// outbound
} NAT;


int numextaddr, numnats, nportranges;
Biobuf *bp;
hosti2a myhosti2a;
//Owners allhosts;
a2imapper a2imap[A2ISZ];
buffring br;
//Timerinfohd tq, evq;
//Msgbuffhd freeMlist;
stable sessiontable;
int verbose;
//Biobuf *Bout;
int natid = 0, msgid = 0;

static void
terminate(NAT *nat, int kill)
{
	close(nat->insidefd);
	nat->insidefd = -1;
	close(nat->insidecfd);
	nat->insidecfd = -1;
	dying = 1;
	if(kill)
		postnote(PNGROUP, getpid(), "die");
}

static void
catchdie(void*, char *msg)
{
	if(strstr(msg, "die") != nil)
		noted(NCONT);
	else
		noted(NDFLT);
}

double
wallClock(void)
{
	static vlong sec0 = 0;
	vlong now;

	now = nsec();
	if(sec0==0){ sec0 = now; }
	return(1e-9*(now-sec0));
}


//-------------------- CHILD PROCESSES --------------------

// write outbound packet
static int
natwrite(NAT *nat, Block *b, int fd)
{
	int len;

	assert(b->next == nil);
	nat->out.packets++;
	len = BLEN(b);
	if(write(fd, b->rp, len) < 0){
		freeb(b);
		return -1;
	}
	if(verbose>2){
		netlog("O write %d\n", len);
	}
	nat->out.uchars += len;
	freeb(b);
	return len;
}

static void
outboundproc(NAT *nat)
{
	Block *b;
	int m, n;
	IP *ip;
	int ind, fd;

	netlog("O nat starting\n");
	while(!dying){
		b = allocb(Buflen);
		n = read(nat->insidefd, b->wp, b->lim-b->wp);
		if(n < 0) {
			netlog("outboundproc: n %d\n", n);
			break;
		}
		// trim packet if there's padding (e.g. from ether)
		ip = (IP*)b->rp;
		m = nhgets(ip->length);
		if(m < n && m > 0)
			n = m;
		b->wp += n;

		if(verbose>2){
			netlog("\nO read %d\n", n);
		}
		if(NATpacket(wallClock()-time0, OUTBOUND, b->rp, &ind) != NAT_PASS){
			freeb(b);
			continue;
		}
		fd = myhosti2a.i2amap[ind].conndfd;
		if(natwrite(nat, b, fd) < 0) {
			netlog("outboundproc: natwrite failed\n");
			break;
		}
	}
	netlog("O nat shuting down\n");
}


// read inbound packet
static Block*
natread(NAT *, int fd, int *nchars)
{
	int n, len;
	Block *b;

	if(dying)
		return nil;
	b = allocb(Buflen);
	len = b->lim - b->wp;
	n = read(fd, b->wp, len);
	if(n <= 0 || n == len){
		freeb(b);
		return nil;
	}
	b->wp += n;
	if(verbose>2){
		netlog("\nI read %d\n", n);
	}
	if(b->rp >= b->wp){
		freeb(b);
		return nil;
	}
	*nchars = n;
	return b;
}

static void
inboundproc(NAT *nat, int addrind)
{
	Block *b;
	int fd = 2, m, n;

	notify(catchdie);
	netlog("I nat %d starting\n", addrind);

	rlock(&(myhosti2a.i2amap[addrind]));
	fd = myhosti2a.i2amap[addrind].conndfd;
	runlock(&(myhosti2a.i2amap[addrind]));

	while(!dying){
		b = natread(nat, fd, &n);
		if(b == nil) {
			netlog("inboundproc%d natread failed\n", addrind);
			break;
		}
		assert(b->next == nil);
		if(NATpacket(wallClock(), 1-OUTBOUND, b->rp, &fd) != NAT_PASS){
			freeb(b);
			continue;
		};
		qlock(&(nat->insidelock));
		m = write(nat->insidefd, b->rp, BLEN(b));
		nat->in.uchars += n;
		nat->in.packets++;
		qunlock(&(nat->insidelock));
		if(m < 0) {
			netlog("write error on insidefd: %r\n");
			freeb(b);
			break;
		}
		if(verbose>2){
			netlog("I write %ld\n", BLEN(b));
		}
		freeb(b);
	}

	netlog("I nat shuting down\n");
	// syslog(0, LOG, "nat shuting down");
	netlog("\t\tnat send = %lud/%lud recv= %lud/%lud",
		nat->out.packets, nat->out.uchars,
		nat->in.packets, nat->in.uchars);
}



//-------------------- MAIN --------------------

static void
setdefroute(char *net, Ipaddr gate)
{
	int fd;
	char path[128];

	snprint(path, sizeof path, "%s/iproute", net);
	fd = open(path, ORDWR);
	if(fd < 0)
		return;
	fprint(fd, "add 0 0 %I", gate);
	close(fd);
}

// open packet interface
static char*
openfds(char *net, Ipaddr addr, Ipaddr raddr, ulong mtu, int *pfd, int *pcfd)
{
	int n, cfd, fd;
	char buf[128];
	char path[128];

	snprint(path, sizeof path, "%s/ipifc/clone", net);
	cfd = open(path, ORDWR);
	if(cfd < 0)
		return "can't open ip interface";
	n = read(cfd, buf, sizeof(buf) - 1);
	if(n <= 0){
		close(cfd);
		return "can't open ip interface";
	}
	buf[n] = 0;
	snprint(path, sizeof path, "%s/ipifc/%s/data", net, buf);
	fd = open(path, ORDWR);
	if(fd < 0){
		close(cfd);
		return "can't open ip interface";
	}
	if(fprint(cfd, "bind pkt") < 0){
		fprint(2, "binding pkt to ip interface: %r");
		return "binding pkt to ip interface";
	}
	*pfd = fd;
	*pcfd = cfd;

	snprint(buf, sizeof(buf), "add %I 255.255.255.255 %I %lud proxy",
		addr, raddr, mtu);
	if(fprint(cfd, "%s", buf) < 0){
		close(cfd);
		return "can't set  addresses";
	}
	if(fprint(cfd, "iprouting 1") < 0){
		close(cfd);
		return "can't set  iprouting";
	}

	return nil; // success
}

// outsideaddr = existing addr of outsidenet
// outsideraddr = new addr obtained by ip/dhcpclient
static void
outsideaddrs(NAT *nat)
{
	Ipifc *ifc;

	// ----- nat->outsideaddr -----
	ifc = readipifc(nat->outsidenet, nil, 0);
	if(ifc == nil){
		netlog("need to ipconfig outside before starting natd\n");
		exits("ipconfig outside");
	}
	memcpy(nat->outsideaddr, ifc->lifc->ip, sizeof(ifc->lifc->ip));
	netlog("outside addr %I\n", nat->outsideaddr);
	return;
}

void
usage(void)
{
	fprint(2, "usage: natd [-v] [-m mtu]\n");
	exits("usage");
}

void
debug(char* net)
{
	Ipifc *ifc;
	Iplifc *t;

	ifc = readipifc(net, nil, 0);
	if (ifc == nil) 	
		netlog("nothing there\n"); 
	else {
		for (t = ifc->lifc; t; t = t->next) {
			ifc->dev[IPaddrlen-1] = '\0';
			netlog("%s ip %V mask %M net %V ", ifc->dev, t->ip, t->mask, t->net);
			netlog("mtu %d \n", ifc->mtu);
		}
	}
}

void
main(int argc, char **argv)
{
	NAT *nat;
	char *p;
	Ipaddr nataddr;
	int i;

	nat = emalloc(sizeof(NAT));
	//nat = malloc(sizeof(NAT));
	nat->mtu = Defmtu;
	nat->insidefd = -1;
	nat->insidecfd = -1;
	fmtinstall('I', eipfmt);
	fmtinstall('E', eipfmt);
	fmtinstall('V', eipfmt);
	fmtinstall('M',eipfmt);

	ARGBEGIN{
	case 'm':
		p = ARGF();
		if(p == nil)
			usage();
		nat->mtu = atoi(p);
		if(nat->mtu < Minmtu)
			nat->mtu = Minmtu;
		if(nat->mtu > Maxmtu)
			nat->mtu = Maxmtu;
		break;
	case 'v':
		verbose++;
		break;
	default:
		fprint(2, "unknown option %c\n", _argc);
		break;
	}ARGEND;

	nat->insidenet = "/net.alt";
	nat->outsidenet = "/net";
	parseip(nat->insideaddr, "10.1.1.1");
	parseip(nat->insideraddr, "10.1.1.2");
	outsideaddrs(nat);

	NATinit("addr");  

	p = openfds(nat->insidenet, nat->insideaddr, nat->insideraddr, nat->mtu,
		&(nat->insidefd), &(nat->insidecfd));
	if(p !=nil){
		fprint(2,"inside openfds failed: %s\n", p);
		exits(p);
	}
	setdefroute(nat->insidenet, nat->insideraddr);

	time0 = wallClock();

	for (i=0; i<numextaddr; i++) {
		v4tov6(nataddr, &(myhosti2a.i2amap[i].nataddr[0]));
		p = openfds(nat->outsidenet, nat->outsideaddr, nataddr, nat->mtu,
			&(myhosti2a.i2amap[i].conndfd), &(myhosti2a.i2amap[i].conncfd));
		if(p !=nil){
			fprint(2,"outside openfds failed: %s\n", p);
			exits(p);
		}

		rfork(RFREND|RFMEM|RFNOTEG|RFNAMEG);
	
		switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
		case -1:
			exits("forking inboundproc");
		case 0:
			inboundproc(nat, i);
			terminate(nat, 1);
			_exits(0);
		}
	}

	switch(rfork(RFPROC|RFMEM|RFNOWAIT)){
	case -1:
		exits("forking outboundproc");
	case 0:
		outboundproc(nat);
		terminate(nat, 1);
		_exits(0);
	}

	debug("/net");
	debug("/net.alt");

	netlog("natd started\n");

	for (;;) {
		sleep(HRTBTINTRVL);
		netlog("heart-beat sender woke up\n");
		writebuff();	
		netlog("heart-beat sender going back to sleep\n");
	}
	exits(0);
}

