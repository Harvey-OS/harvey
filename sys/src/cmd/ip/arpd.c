/*
 * The arpd is very ethernet specific like most of the ip code.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include "/sys/src/9/port/arp.h"
#include <ip.h>

#define ET_IP		0x800
#define WRITESTR(fd, s)	if(write(fd, s, sizeof(s)-1) < 0) { perror(s); exits("writestr"); }

typedef struct Pend	Pend;
typedef struct Ether	Ether;

struct Ether
{
	uchar	d[6];
	uchar	s[6];
	uchar	t[6];
	uchar	data[1600];
};

struct Pend
{
	Pend	*next;
	int	pktlen;
	char	data[1600];
};

void	error(char*);
void	hnputs(uchar*, ushort);
ushort	nhgets(uchar*);
void	arp(uchar*);
void	arpip(uchar*, int);
void	ippending(uchar*, uchar*);

Arppkt	out;
Arppkt	reply;
Pend   *phead;
Pend   *ptail;
int 	debug;
int 	prom;
int	arpdev;
uchar 	bcast[6];
char	*etherdev = "ether";
char	arplog[] = "arpd";
Ndb	*db;
int	edata, ectl;
uchar	eaddr[6];
uchar	ipaddr[4];

void
main(int argc, char **argv)
{
	uchar buf[1600];
	char name[64];
	int n;
	ushort type;
	Arppkt *ap;

	memset(bcast, 0xff, sizeof(bcast));	/* broadcast address */

	ARGBEGIN{
	case 'p':
		prom = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'e':
		etherdev = ARGF();
		break;
	case 'b':
		parseether(bcast, ARGF());
		break;
	default:
		fprint(2, "usage: arpd [-d] [-p] [-e device] [-b broadcast address]\n");
		exits("usage");
	}ARGEND
	USED(argc); USED(argv);

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	if(myipaddr(ipaddr, "/net/udp") < 0)
		error("can't get my ip address");
	sprint(name, "/net/%s", etherdev);
	if(myetheraddr(eaddr, name) < 0)
		error("can't get my ether address");

	db = ndbopen(0);
	if(db == 0)
		error("can't open the database");

	edata = dial(netmkaddr("0x806", etherdev, 0), 0, 0, &ectl);
	if(edata < 0)
		error("can't open ethernet");
	WRITESTR(ectl, "push arp");
	WRITESTR(ectl, "arpd");

	if(bind("#a", "/net", MBEFORE) < 0)
		error("binding /net/arp");
	if((arpdev = open("/net/arp/data", ORDWR)) < 0)
		error("open /net/arp/data");

	if(debug)
		print("arpd: started for %I %E\n", ipaddr, eaddr);

	for(;;) {
		n = read(edata, buf, sizeof(buf));
		if(n < 0)
			error("read loop");

		ap = (Arppkt *)buf;
		type = (ap->type[0]<<8)|ap->type[1];
		switch(type) {
		default:
			syslog(0, arplog, "Unknown packet type %lux", ap->type);
			break;
		case ET_IP:			/* An ip packet which needs arping */
			arpip(buf, n);
			break;
		case ET_ARP:			/* An arp packet from the network */
			arp(buf);
			break;
		}
	}
}

void
arpip(uchar *pkt, int pktlen)
{
	Pend *p;
	int n;

	if(debug)
		print("arpd: request for %I\n", pkt);

	/* The arp stream packs the routed ip address into the ethernet destination
	 * (first four bytes) */
	memmove(out.tpa, pkt, sizeof(out.tpa));
	memmove(out.d, bcast, sizeof(out.d));
	memmove(out.spa, ipaddr, sizeof(ipaddr));
	memmove(out.sha, eaddr, sizeof(eaddr));

	hnputs(out.type, ET_ARP);
	hnputs(out.hrd, 1);
	hnputs(out.pro, ET_IP);
	out.hln = sizeof(out.sha);
	out.pln = sizeof(out.spa);
	hnputs(out.op, ARP_REQUEST);

	n = write(edata, &out, sizeof(out));
	if(n < 0)
		perror("write arp cache request");

	p = malloc(sizeof(Pend));
	p->pktlen = pktlen;
	p->next = 0;
	memmove(p->data, pkt, pktlen);
	if(phead)
		ptail->next = p;
	else
		phead = p;
	ptail = p;
}

void
arp(uchar *pkt)
{
	Arppkt *ap;
	Arpentry entry;
	Ndbtuple *t;
	char ebuf[Ndbvlen];
	char ipbuf[Ndbvlen];
	Ndbs s;

	ap = (Arppkt*)pkt;

	if(nhgets(ap->pro) != ET_IP || nhgets(ap->hrd) != 1 || 
     	   ap->pln != sizeof(ap->tpa) || ap->hln != sizeof(ap->tha)) {
		print("arpd: dropped\n");
		return;
	}

	if(debug)
		print("arpd: for %I\n", ap->tpa);

	switch(nhgets(ap->op)) {
	default:
		syslog(0, arplog, "unknown arp operation %d", nhgets(ap->op));
		break;
	case ARP_REQUEST:
		hnputs(reply.type, ET_ARP);
		hnputs(reply.hrd, 1);
		hnputs(reply.pro, ET_IP);
		reply.hln = sizeof(reply.sha);
		reply.pln = sizeof(reply.spa);
		hnputs(reply.op, ARP_REPLY);
		if(memcmp(ap->tpa, ipaddr, sizeof(ipaddr)) == 0) {
			memmove(reply.tha, ap->sha, sizeof(out.tha));
			memmove(reply.tpa, ap->spa, sizeof(out.tpa));
			memmove(reply.sha, eaddr, sizeof(eaddr));
			memmove(reply.spa, ipaddr, sizeof(ipaddr));
			if(debug)
				print("arpd: self\n");
		}
		else if(prom == 1) {
			sprint(ipbuf, "%I", ap->tpa);
			t = ndbgetval(db, &s, "ip", ipbuf, "ether", ebuf);
			if(t == 0) {
				if(debug)
					print("arpd: failed %I\n", ap->tpa);
				break;
			}
			ndbfree(t);
			parseether(reply.sha, ebuf);
			memmove(reply.tha, ap->sha, sizeof(out.tha));
			memmove(reply.tpa, ap->spa, sizeof(out.tpa));
			memmove(reply.spa, ap->tpa, sizeof(out.tpa));
/*			syslog(0, arplog, "prom"); /**/
		}
		else
			break;

		if(debug) {
			print("arpd: reply to %E\n", ap->s);
			print("arpd: s %I %E t %I %E\n", reply.spa, reply.sha, reply.tpa, reply.tha);
		}
		memmove(reply.d, ap->s, sizeof(ap->s));
		if(write(edata, &reply, ARPSIZE) < 0)
			perror("write arp reply");
		break;
	case ARP_REPLY:
		if(debug)
			print("arpd: cache %E %I\n", ap->sha, ap->spa);

		memmove(entry.etaddr, ap->sha, sizeof(entry.etaddr));
		memmove(entry.ipaddr, ap->spa, sizeof(entry.ipaddr));
		if(write(arpdev, &entry, sizeof(entry)) < 0)
			perror("write arp entry");
		ippending(entry.ipaddr, entry.etaddr);
		break;
	}
}

void
ippending(uchar *ip, uchar *et)
{
	Pend **l, *f;
	Ether *e;

	l = &phead;
	for(f = *l; f; f = *l) {
		e = (Ether*)f->data;
		if(memcmp(e->d, ip, 4) == 0) {
			memmove(e->d, et, sizeof(e->d));
			write(edata, f->data, f->pktlen);
			*l = f->next;
			free(f);
		}
		else
			l = &f->next;
	}
}

void
error(char *s)
{
	char buf[64];

	errstr(buf);
	fprint(2, "arpd: %s: %s\n", s, buf);
	exits(0);
}

ushort
nhgets(uchar *val)
{
	return (val[0]<<8) | val[1];
}

void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}
