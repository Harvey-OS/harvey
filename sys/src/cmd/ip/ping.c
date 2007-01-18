/* ping for ip v4 and v6 */
#include <u.h>
#include <libc.h>
#include <ip.h>

enum
{
	/* Packet Types */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	EchoRequest	= 8,
	TimeExceed	= 11,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,

	EchoReplyV6	= 129,
	EchoRequestV6	= 128,

	MAXMSG		= 32,
	SLEEPMS		= 1000,

	SECOND		= 1000000000LL,
	MINUTE		= 60*SECOND,
};

typedef struct Icmp Icmp;
struct Icmp
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	ipcksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
};

typedef struct Icmp6 Icmp6;
struct Icmp6
{
	uchar	vcf[4];
	uchar	ploadlen[2];
	uchar	proto;
	uchar	ttl;
	uchar	src[16];		/* Ip source */
	uchar	dst[16];		/* Ip destination */
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
};

typedef struct Req Req;
struct Req
{
	ushort	seq;	/* sequence number */
	vlong	time;	/* time sent */
	vlong	rtt;
	int	ttl;
	int	replied;
	Req	 *next;
};

typedef struct {
	char	*net;
	int	echoreply;
	unsigned icmphdrsz;
	int	(*getttl)(void *v);
	int	(*getseq)(void *v);
	void	(*putseq)(void *v, ushort seq);
	int	(*gettype)(void *v);
	void	(*settype)(void *v);
	int	(*getcode)(void *v);
	void	(*setcode)(void *v);
	void	(*prreply)(Req *r, void *v);
	void	(*prlost)(ushort seq, void *v);
} Proto;

Req	*first;		/* request list */
Req	*last;		/* ... */
Lock	listlock;

char *argv0;
int debug;
int quiet;
int lostonly;
int lostmsgs;
int rcvdmsgs;
int done;
int rint;
vlong sum;
ushort firstseq;
int addresses;
int flood;

void lost(Req*, void*);
void reply(Req*, void*);

static void
usage(void)
{
	fprint(2,
	    "usage: %s [-6alq] [-s msgsize] [-i millisecs] [-n #pings] dest\n",
		argv0);
	exits("usage");
}

static void
catch(void *a, char *msg)
{
	USED(a);
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else if(strstr(msg, "die"))
		exits("errors");
	else
		noted(NDFLT);
}


static int
getttl4(void *v)
{
	return ((Icmp *)v)->ttl;
}

static int
getttl6(void *v)
{
	return ((Icmp6 *)v)->ttl;
}

static int
getseq4(void *v)
{
	return nhgets(((Icmp *)v)->seq);
}

static int
getseq6(void *v)
{
	Icmp6 *ip6 = v;

	return ip6->seq[1]<<8 | ip6->seq[0];
}

static void
putseq4(void *v, ushort seq)
{
	hnputs(((Icmp *)v)->seq, seq);
}

static void
putseq6(void *v, ushort seq)
{
	((Icmp6 *)v)->seq[0] = seq;
	((Icmp6 *)v)->seq[1] = seq>>8;
}

static int
gettype4(void *v)
{
	return ((Icmp *)v)->type;
}

static int
gettype6(void *v)
{
	return ((Icmp6 *)v)->type;
}

static void
settype4(void *v)
{
	((Icmp *)v)->type = EchoRequest;
}

static void
settype6(void *v)
{
	((Icmp6 *)v)->type = EchoRequestV6;
}

static int
getcode4(void *v)
{
	return ((Icmp *)v)->code;
}

static int
getcode6(void *v)
{
	return ((Icmp6 *)v)->code;
}

static void
setcode4(void *v)
{
	((Icmp *)v)->code = 0;
}

static void
setcode6(void *v)
{
	((Icmp6 *)v)->code = 0;
}

static void
prlost4(ushort seq, void *v)
{
	Icmp *ip4 = v;

	print("lost %ud: %V->%V\n", seq, ip4->src, ip4->dst);
}

static void
prlost6(ushort seq, void *v)
{
	Icmp6 *ip6 = v;

	print("lost %ud: %I->%I\n", seq, ip6->src, ip6->dst);
}

static void
prreply4(Req *r, void *v)
{
	Icmp *ip4 = v;

	print("%ud: %V->%V rtt %lld µs, avg rtt %lld µs, ttl = %d\n",
		r->seq - firstseq, ip4->src, ip4->dst, r->rtt, sum/rcvdmsgs,
		r->ttl);
}

static void
prreply6(Req *r, void *v)
{
	Icmp *ip6 = v;

	print("%ud: %I->%I rtt %lld µs, avg rtt %lld µs, ttl = %d\n",
		r->seq - firstseq, ip6->src, ip6->dst, r->rtt, sum/rcvdmsgs,
		r->ttl);
}

static Proto v4pr = {
	"icmp",
	EchoReply,
	sizeof(Icmp),
	getttl4,
	getseq4,
	putseq4,
	gettype4,
	settype4,
	getcode4,
	setcode4,
	prreply4,
	prlost4,
};
static Proto v6pr = {
	"icmpv6",
	EchoReplyV6,
	sizeof(Icmp6),
	getttl6,
	getseq6,
	putseq6,
	gettype6,
	settype6,
	getcode6,
	setcode6,
	prreply6,
	prlost6,
};

static Proto *proto = &v4pr;

void
clean(ushort seq, vlong now, void *v)
{
	Req **l, *r;

	lock(&listlock);
	last = nil;
	for(l = &first; *l; ){
		r = *l;

		if(v && r->seq == seq){
			r->rtt = now-r->time;
			r->ttl = (*proto->getttl)(v);
			reply(r, v);
		}

		if(now-r->time > MINUTE){
			*l = r->next;
			r->rtt = now-r->time;
			if(v)
				r->ttl = (*proto->getttl)(v);
			if(r->replied == 0)
				lost(r, v);
			free(r);
		}else{
			last = r;
			l = &r->next;
		}
	}
	unlock(&listlock);
}

void
sender(int fd, int msglen, int interval, int n)
{
	int i, extra;
	ushort seq;
	char buf[64*1024+512];
	Req *r;

	srand(time(0));
	firstseq = seq = rand();

	for(i = proto->icmphdrsz; i < msglen; i++)
		buf[i] = i;
	(*proto->settype)(buf);
	(*proto->setcode)(buf);

	for(i = 0; i < n; i++){
		if(i != 0){
			extra = rint? nrand(interval): 0;
			sleep(interval + extra);
		}
		r = malloc(sizeof *r);
		if (r == nil)
			continue;
		(*proto->putseq)(buf, seq);
		r->seq = seq;
		r->next = nil;
		r->replied = 0;
		r->time = nsec();	/* avoid early free in reply! */
		lock(&listlock);
		if(first == nil)
			first = r;
		else
			last->next = r;
		last = r;
		unlock(&listlock);
		r->time = nsec();
		if(write(fd, buf, msglen) < msglen){
			fprint(2, "%s: write failed: %r\n", argv0);
			return;
		}
		seq++;
	}
	done = 1;
}

void
rcvr(int fd, int msglen, int interval, int nmsg)
{
	int i, n, munged;
	ushort x;
	vlong now;
	uchar buf[64*1024+512];
	Req *r;

	sum = 0;
	while(lostmsgs+rcvdmsgs < nmsg){
		alarm((nmsg-lostmsgs-rcvdmsgs)*interval+5000);
		n = read(fd, buf, sizeof buf);
		alarm(0);
		now = nsec();
		if(n <= 0){	/* read interrupted - time to go */
			clean(0, now+MINUTE, nil);
			continue;
		}
		if(n < msglen){
			print("bad len %d/%d\n", n, msglen);
			continue;
		}
		munged = 0;
		for(i = proto->icmphdrsz; i < msglen; i++)
			if(buf[i] != (uchar)i)
				munged++;
		if(munged)
			print("corrupted reply\n");
		x = (*proto->getseq)(buf);
		if((*proto->gettype)(buf) != proto->echoreply ||
		   (*proto->getcode)(buf) != 0) {
			print("bad sequence/code/type %d/%d/%d\n",
				(*proto->gettype)(buf), (*proto->getcode)(buf),
				x);
			continue;
		}
		clean(x, now, buf);
	}
	
	lock(&listlock);
	for(r = first; r; r = r->next)
		if(r->replied == 0)
			lostmsgs++;
	unlock(&listlock);

	if(lostmsgs)
		print("%d out of %d messages lost\n", lostmsgs,
			lostmsgs+rcvdmsgs);
}

void
main(int argc, char **argv)
{
	int fd, msglen, interval, nmsg;
	char *ds;

	nsec();		/* make sure time file is already open */

	fmtinstall('V', eipfmt);

	msglen = interval = 0;
	nmsg = MAXMSG;
	ARGBEGIN {
	case '6':
		proto = &v6pr;
		break;
	case 'a':
		addresses = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'f':
		flood = 1;
		break;
	case 'i':
		interval = atoi(EARGF(usage()));
		break;
	case 'l':
		lostonly++;
		break;
	case 'n':
		nmsg = atoi(EARGF(usage()));
		break;
	case 'q':
		quiet = 1;
		break;
	case 'r':
		rint = 1;
		break;
	case 's':
		msglen = atoi(EARGF(usage()));
		break;
	default:
		usage();
		break;
	} ARGEND;

	if(msglen < proto->icmphdrsz)
		msglen = proto->icmphdrsz;
	if(msglen < 64)
		msglen = 64;
	if(msglen >= 65*1024)
		msglen = 65*1024-1;
	if(interval <= 0 && !flood)
		interval = SLEEPMS;

	if(argc < 1)
		usage();

	notify(catch);

	ds = netmkaddr(argv[0], proto->net, "1");
	fd = dial(ds, 0, 0, 0);
	if(fd < 0){
		fprint(2, "%s: couldn't dial %s: %r\n", argv0, ds);
		exits("dialing");
	}

	print("sending %d %d byte messages %d ms apart to %s\n",
		nmsg, msglen, interval, ds);

	switch(rfork(RFPROC|RFMEM|RFFDG)){
	case -1:
		fprint(2, "%s: can't fork: %r\n", argv0);
		/* fallthrough */
	case 0:
		rcvr(fd, msglen, interval, nmsg);
		exits(0);
	default:
		sender(fd, msglen, interval, nmsg);
		wait();
		exits(lostmsgs ? "lost messages" : "");
	}
}

void
reply(Req *r, void *v)
{
	r->rtt /= 1000LL;
	sum += r->rtt;
	if(!r->replied)
		rcvdmsgs++;
	if(!quiet && !lostonly)
		if(addresses)
			(*proto->prreply)(r, v);
		else
			print("%ud: rtt %lld µs, avg rtt %lld µs, ttl = %d\n",
				r->seq - firstseq, r->rtt, sum/rcvdmsgs, r->ttl);
	r->replied = 1;
}

void
lost(Req *r, void *v)
{
	if(!quiet)
		if(addresses && v != nil)
			(*proto->prlost)(r->seq - firstseq, v);
		else
			print("lost %ud\n", r->seq - firstseq);
	lostmsgs++;
}
