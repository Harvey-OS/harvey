#include <u.h>
#include <libc.h>
#include <ip.h>

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

enum
{			/* Packet Types */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	EchoRequest	= 8,
	TimeExceed	= 11,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,

	ICMP_IPSIZE	= 20,
	ICMP_HDRSIZE	= 8,

	MAXMSG		= 32,
	SLEEPMS		= 1000,
};

typedef struct Req Req;
struct Req
{
	ushort	seq;	// sequence number
	vlong	time;	// time sent
	vlong	rtt;
	int	ttl;
	int	replied;
	Req	 *next;
};
Req	*first;		// request list
Req	*last;		// ...
Lock	listlock;

char *argv0;
int debug;
int quiet;
int lostonly;
int lostmsgs;
int rcvdmsgs;
int done;
vlong sum;
ushort firstseq;
int addresses;

void usage(void);
void lost(Req*, Icmp*);
void reply(Req*, Icmp*);

#define SECOND 1000000000LL
#define MINUTE (60LL*SECOND)

static void
catch(void *a, char *msg)
{
	USED(a);
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

void
clean(ushort seq, vlong now, Icmp *ip)
{
	Req **l, *r;

	lock(&listlock);
	for(l = &first; *l; ){
		r = *l;

		if(r->seq == seq){
			r->rtt = now-r->time;
			r->ttl = ip->ttl;
			reply(r, ip);
		}

		if(now-r->time > MINUTE){
			*l = r->next;
			r->rtt = now-r->time;
			r->ttl = ip->ttl;
			if(r->replied == 0)
				lost(r, ip);
			free(r);
		} else {
			last = r;
			l = &(r->next);
		}
	}
	unlock(&listlock);
}

void
sender(int fd, int msglen, int interval, int n)
{
	char buf[64*1024+512];
	Icmp *ip;
	int i;
	Req *r;
	ushort seq;

	ip = (Icmp*)buf;

	srand(time(0));
	firstseq = seq = rand();

	for(i = 32; i < msglen; i++)
		buf[i] = i;
	ip->type = EchoRequest;
	ip->code = 0;

	for(i = 0; i < n; i++){
		if(i != 0)
			sleep(interval);
		r = malloc(sizeof *r);
		if(r != nil){
			ip->seq[0] = seq;
			ip->seq[1] = seq>>8;
			r->seq = seq;
			r->next = nil;
			lock(&listlock);
			if(first == nil)
				first = r;
			else
				last->next = r;
			last = r;
			unlock(&listlock);
			r->replied = 0;
			r->time = nsec();
			if(write(fd, ip, msglen) < msglen){
				fprint(2, "%s: write failed: %r\n", argv0);
				return;
			}
			seq++;
		}
	}
	done = 1;
}

void
rcvr(int fd, int msglen, int interval, int nmsg, int senderpid)
{
	uchar buf[64*1024+512];
	Icmp *ip;
	ushort x;
	int i, n, munged;
	vlong now;
	Req *r;

	ip = (Icmp*)buf;
	sum = 0;

	while(!done || first != nil){
		alarm((nmsg-lostmsgs-rcvdmsgs)*interval+5000);
		n = read(fd, buf, sizeof(buf));
		alarm(0);
		now = nsec();
		if(n <= 0){
			print("read: %r\n");
			break;
		}
		if(n < msglen){
			print("bad len %d/%d\n", n, msglen);
			continue;
		}
		munged = 0;
		for(i = 32; i < msglen; i++)
			if(buf[i] != (i&0xff))
				munged++;
		if(munged)
			print("currupted reply\n");
		x = (ip->seq[1]<<8)|ip->seq[0];
		if(ip->type != EchoReply || ip->code != 0) {
			print("bad sequence/code/type %d/%d/%d\n",
				ip->type, ip->code, x);
			continue;
		}
		clean(x, now, ip);
	}
	
	lock(&listlock);
	for(r = first; r; r = r->next)
		if(r->replied == 0)
			lostmsgs++;
	unlock(&listlock);

	if(lostmsgs)
		print("%d out of %d messages lost\n", lostmsgs, lostmsgs+rcvdmsgs);
	postnote(PNPROC, senderpid, "die");
}

void
usage(void)
{
	fprint(2, "usage: %s [-alq] [-s msgsize] [-i millisecs] [-n #pings] destination\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd;
	int msglen, interval, nmsg;
	int pid;

	nsec();		/* make sure time file is already open */

	fmtinstall('V', eipfmt);

	msglen = interval = 0;
	nmsg = MAXMSG;
	ARGBEGIN {
	case 'l':
		lostonly++;
		break;
	case 'd':
		debug++;
		break;
	case 's':
		msglen = atoi(ARGF());
		break;
	case 'i':
		interval = atoi(ARGF());
		break;
	case 'n':
		nmsg = atoi(ARGF());
		break;
	case 'a':
		addresses = 1;
		break;
	case 'q':
		quiet = 1;
		break;
	} ARGEND;
	if(msglen < 32)
		msglen = 64;
	if(msglen >= 65*1024)
		msglen = 65*1024-1;
	if(interval <= 0)
		interval = SLEEPMS;

	if(argc < 1)
		usage();

	notify(catch);

	fd = dial(netmkaddr(argv[0], "icmp", "1"), 0, 0, 0);
	if(fd < 0){
		fprint(2, "%s: couldn't dial: %r\n", argv0);
		exits("dialing");
	}

	print("sending %d %d byte messages %d ms apart\n", nmsg, msglen, interval);

	pid = getpid();
	switch(rfork(RFPROC|RFMEM|RFFDG)){
	case -1:
		fprint(2, "%s: can't fork: %r\n", argv0);
	case 0:
		rcvr(fd, msglen, interval, nmsg, pid);
		exits(0);
	default:
		sender(fd, msglen, interval, nmsg);
		wait();
		exits(lostmsgs ? "lost messages" : "");
	}
}

void
reply(Req *r, Icmp *ip)
{
	rcvdmsgs++;
	r->rtt /= 1000LL;
	sum += r->rtt;
	if(!quiet && !lostonly){
		if(addresses)
			print("%ud: %V->%V rtt %lld µs, avg rtt %lld µs, ttl = %d\n",
				r->seq-firstseq,
				ip->src, ip->dst,
				r->rtt, sum/rcvdmsgs, r->ttl);
		else
			print("%ud: rtt %lld µs, avg rtt %lld µs, ttl = %d\n",
				r->seq-firstseq,
				r->rtt, sum/rcvdmsgs, r->ttl);
	}
	r->replied = 1;
}

void
lost(Req *r, Icmp *ip)
{
	if(!quiet){
		if(addresses)
			print("lost %ud: %V->%V avg rtt %lld µs\n",
				r->seq-firstseq,
				ip->src, ip->dst,
				sum/rcvdmsgs);
		else
			print("lost %ud: avg rtt %lld µs\n",
				r->seq-firstseq,
				sum/rcvdmsgs);
	}
	lostmsgs++;
}
