#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static struct {
	int thread;
	QLock;
	int fd;
} udp = { -1 };

typedef struct Listen Listen;

struct Listen {
	NbName to;
	int (*deliver)(void *magic, NbDgram *s);
	void *magic;
	Listen *next;
};

static struct {
	QLock;
	Listen *head;
} listens;

static void
udplistener(void *)
{
//print("udplistener - starting\n");
	for (;;) {
		uchar msg[Udphdrsize + 576];
		int len = read(udp.fd, msg, sizeof(msg));
		if (len < 0)
			break;
		if (len >= nbudphdrsize) {
			NbDgram s;
//			Udphdr *uh;
			uchar *p;
			int n;

//			uh = (Udphdr*)msg;
			p = msg + nbudphdrsize;
			len -= nbudphdrsize;
			n = nbdgramconvM2S(&s, p, p + len);
			if (n) {
				switch (s.type) {
				case NbDgramError:
					print("nbdgramlisten: error: ip %I port %d code 0x%.2ux\n", s.srcip, s.srcport, s.error.code);
					break;
				case NbDgramDirectUnique:
				case NbDgramDirectGroup:
				case NbDgramBroadcast: {
					int delivered = 0;
					Listen **lp, *l;
					if ((s.flags & NbDgramMore) || s.datagram.offset != 0)
						break;
					if (!nbnameisany(s.datagram.dstname)
						&& !nbnametablefind(s.datagram.dstname, 0)) {
/* - only do this if a broadcast node, and can tell when packets are broadcast...
						s.flags &= 3;
						ipmove(s.srcip, nbglobals.myipaddr);
						s.srcport = NbDgramPort;
						s.type = NbDgramError;	
						s.error.code = NbDgramErrorDestinationNameNotPresent;
						nbdgramsendto(uh->raddr, nhgets(uh->rport), &s);
*/
						break;
					}
					qlock(&listens);
					for (lp = &listens.head; (l = *lp) != nil;) {
						if (nbnameisany(l->to) || nbnameequal(l->to, s.datagram.dstname)) {
							switch ((*l->deliver)(l->magic, &s)) {
							case 0:
								delivered = 1;
								/* fall through */
							case -1:
								*lp = l->next;
								free(l);
								continue;
							default:
								delivered = 1;
								break;
							}
						}
						lp = &l->next;
					}
					qunlock(&listens);
					USED(delivered);
				}
				default:
					;
				}
			}
		}
	}
print("udplistener - exiting\n");
	qlock(&udp);
	udp.thread = -1;
	qunlock(&udp);
}

static char *
startlistener(void)
{
	qlock(&udp);
	if (udp.thread < 0) {
		char *e;
		e = nbudpannounce(NbDgramPort, &udp.fd);
		if (e) {
			qunlock(&udp);
			return e;
		}
		udp.thread = proccreate(udplistener, nil, 16384);
	}
	qunlock(&udp);
	return nil;
}

char *
nbdgramlisten(NbName to, int (*deliver)(void *magic, NbDgram *s), void *magic)
{
	Listen *l;
	char *e;
	nbnametablefind(to, 1);
	e = startlistener();
	if (e)
		return e;
	l = nbemalloc(sizeof(Listen));
	nbnamecpy(l->to, to);
	l->deliver = deliver;
	l->magic = magic;
	qlock(&listens);
	l->next = listens.head;
	listens.head = l;
	qunlock(&listens);
	return 0;
}

int
nbdgramsendto(uchar *ipaddr, ushort port, NbDgram *s)
{
	Udphdr *u;
	uchar msg[NbDgramMaxPacket + Udphdrsize];
	int l;
	int rv;
	char *e;

	e = startlistener();
	if (e != nil)
		return 0;

	l = nbdgramconvS2M(msg + nbudphdrsize, msg + sizeof(msg), s);
	if (l == 0) {
		print("conv failed\n");
		return 0;
	}
	u = (Udphdr *)msg;
	ipmove(u->laddr, nbglobals.myipaddr);
	hnputs(u->lport, NbDgramPort);
	ipmove(u->raddr, ipaddr);
	hnputs(u->rport, port);
//nbdumpdata(msg, l + nbudphdrsize);
//print("transmitting\n");
	rv = write(udp.fd, msg, l + nbudphdrsize);
//print("rv %d l %d hdrsize %d error %r\n", rv, l, nbudphdrsize);
	return rv == l + nbudphdrsize;
}

static struct {
	Lock;
	ushort id;
} id;

static ushort
nextdgramid(void)
{
	ushort v;
	lock(&id);
	v = id.id++;
	unlock(&id);
	return v;
}

int
nbdgramsend(NbDgramSendParameters *p, uchar *data, long datalen)
{
	NbDgram s;
	uchar dstip[IPaddrlen];
	s.type = p->type;
	switch (p->type) {
	case NbDgramBroadcast:
	case NbDgramDirectGroup:
		ipmove(dstip, nbglobals.bcastaddr);
		break;
	case NbDgramDirectUnique:
		if (!nbnameresolve(p->to, dstip)) {
			werrstr("nbdgramsend: name resolution failed");
			return 0;
		}
		break;
	default:
		werrstr("nbdgramsend: illegal datagram type");
		return 0;
	}
	s.flags = NbDgramFirst;
	s.id = nextdgramid();
	ipmove(s.srcip, nbglobals.myipaddr);
	s.srcport = NbDgramPort;
	s.datagram.offset = 0;
	s.datagram.data = data;
	s.datagram.length = datalen;
	nbnamecpy(s.datagram.dstname, p->to);
	nbnamecpy(s.datagram.srcname, nbglobals.myname);
	return nbdgramsendto(dstip, NbDgramPort, &s);
}
