#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include <ctype.h>
#include "icmp.h"
#define	max(a,b)	((a) > (b) ? (a) : (b))
#define	Bufsize	5000

int	nsent;	/* no of ICMP echo reqs sent */
int	done;
char	icmpmsg[] = "0123456789abcdefghijklmnopqrstuvwxy";
long	*tsent;	/* array of send times for packets */

void
fatal(char *fmt, ...)
{
	int n;
	char err[256];

	n = doprint(err, err+sizeof(err), fmt, &fmt+1) - err;
	err[n] = '\n';
	write(2, err, n+1);
	exits("fatal error");
}

static void
timer(void *a, char *c)
{
	USED(a);
	if(strcmp(c, "alarm") == 0 || strcmp(c, "interrupt") == 0) {
		done = 1;
		noted(NCONT);
	}
	noted(NDFLT);
}

/*
 * get real time in milliseconds
 */
long
mtime(void)
{
	char b[20];
	static int f = -1;

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("/dev/msec", OREAD|OCEXEC);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	return atol(b);
}

void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

ushort
nhgets(uchar *ptr)
{
	return (ptr[0] << 8) + ptr[1];
}

/*
 * send an ICMP echo request out
 * return sequence number
 */
int
sendping(int fd, uchar hostip[])
{
	static int seq = 0;
	uchar	outbuf[Bufsize];
	Icmp	*q;
	long	n;

	q = (Icmp *) outbuf;
	memcpy(q->dst, hostip, sizeof(q->dst));
	q->proto = ICMP_PROTO;
	q->type = EchoRequest;
	q->code = 0;
	hnputs(q->seq, seq);
	memcpy(q->data, icmpmsg, strlen(icmpmsg));
	n = ICMPHDR + strlen(icmpmsg);
	if(write(fd, outbuf, n + ETHERIPHDR) < 0)
		fatal("write: %r");
	return seq++;
}

void
sendpings(int fd, uchar hostip[], long nsend, long secs)
{
	while(done == 0 && nsend > 0) {
		tsent[sendping(fd, hostip)] = mtime();
		++nsent;
		--nsend;
		sleep(secs);
	}
}

/*
 * scan icmp packets and measure round trip time
 */
void
waitforreply(int fd, uchar hostip[])
{
	long	n, recvtime;
	uchar	inbuf[Bufsize];
	Icmp	*p;

	while(done == 0 && (n = read(fd, inbuf, sizeof(inbuf))) > 0) {
		recvtime = mtime();
		p = (Icmp*) inbuf;
		n -= ETHERIPHDR;
		switch(p->type) {
		case EchoReply:
			if(memcmp(p->src, hostip, sizeof(p->src)) != 0)
				continue;
			if(memcmp(p->data, icmpmsg, n - ICMPHDR) != 0)
				continue;
			print("ICMP: src=%I dst=%I ttl=%d seq=%d msecs=%d\n",
				p->src, p->dst, (int) p->ttl,
				nhgets(p->seq),
				recvtime - tsent[nhgets(p->seq)]);
			break;
		case EchoRequest:
			break;
		default:
			print("Unknown ICMP type %d\n", p->type);
		}
	}
}

void
usage(void)
{
	fatal("Usage: ping [-c count] [-s millisecs] host");
}

void
main(int argc, char *argv[])
{
	int	fd;
	long	npings, secs;
	Ndb	*db;
	Ipinfo	host;

	npings = 10;
	nsent = done = 0;
	secs = 1000;
	ARGBEGIN {
	case 's':
		secs = atoi(ARGF());
		break;
	case 'c':
		npings = atoi(ARGF());
		break;
	default:
		usage();
	} ARGEND;
	if(argc != 1)
		usage();

	db = ndbopen(0);
	if(db == 0)
		fatal("can't open database: %r");

	if(ipinfo(db, 0, isdigit(*argv[0]) ? argv[0] : 0,
			 isalpha(*argv[0]) ? argv[0] : 0, &host) < 0)
		fatal("Cannot find %s in database: %r", argv[0]);

	fmtinstall('I', eipconv);
	notify(timer);
	tsent = (long *) malloc(npings * sizeof(long));
	if(tsent == 0)
		fatal("malloc failed: %r");
	fd = open("/net/icmp", ORDWR);
	if(fd < 0)
		fatal("open: /net/icmp: %r");
	switch(rfork(RFPROC|RFMEM)) {
	case -1:
		fatal("rfork: %r");
	case 0:
		sendpings(fd, host.ipaddr, npings, secs);
		rendezvous(0, 0);
		break;
	default:
		alarm(max(secs,500)*npings);
		waitforreply(fd, host.ipaddr);
		rendezvous(0, 0);
	}
	exits(0);
}
