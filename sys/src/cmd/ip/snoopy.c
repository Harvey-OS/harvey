#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <ndb.h>
#include <ip.h>
#include <ctype.h>
#include "arp.h"
#include "bootp.h"

/*
 *  ether packet
 */
typedef struct Etherpkt	Etherpkt;
struct Etherpkt {
	uchar d[6];
	uchar s[6];
	uchar type[2];
	char data[1500];
};
#define	ETHERMINTU	60	/* minimum transmit size */
#define	ETHERMAXTU	1514	/* maximum transmit size */
#define ETHERHDRSIZE	14	/* size of an ethernet header */

/*
 *  ip packets
 */
typedef struct Ippkt	Ippkt;
struct Ippkt
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
	char	data[1];
};

#define IP_HDRSIZE	20
#define IP_UDPPROTO	17
#define IP_RUDPPROTO	254
#define IP_TCPPROTO	6
#define	IP_ILPROTO	40
#define	IP_GREPROTO	47
#define	IP_ESPPROTO	50
#define	IP_ICMPPROTO	1
#define IP_OSPFPROTO	89
#define	IP_DF		0x4000
#define	IP_MF		0x2000

/*
 *  name translation caches (one per name space)
 */
typedef struct Name	Name;
struct Name
{
	char	before[Ndbvlen+1];
	char	after[Ndbvlen+1];
};

enum
{
	Ncache=	256,	/* name cache entries */
};
typedef struct NS	NS;
struct NS
{
	char	*attr;
	int	n;
	Name	cache[Ncache];
};

NS	ipns = { "ip", 0 };
NS	etns = { "ether", 0 };

#define NetS(x) (((x)[0]<<8) | (x)[1])
#define Net3(x) (((x)[0]<<16) | ((x)[1]<<8) | (x)[2])
#define NetL(x) (((x)[0]<<24) | ((x)[1]<<16) | ((x)[2]<<8) | (x)[3])

/*
 *  run flags
 */
int aflag;
int bflag;
int cflag;
int eflag;
int Eflag;
int flag9;
int gflag;
int iflag;
int lflag;
int nflag;
int oflag;
int pflag = 1;
int rflag;
int tflag;
int uflag;
int xflag;
int special;
int punt;
int timefd;
Biobuf bout;
int fromsniffer;

#define DUMPL 20
int dumpl = DUMPL;
char *netroot;
uchar ipfilter[IPaddrlen];

/*
 *  predeclared
 */
int	sprintbootp(void*, char*, int);
int	sprinticmp(void*, char*, int);
int	sprintudp(void*, char*, int);
int	sprintrudp(void*, char*, int);
int	sprintil(void*, char*, int);
int	sprinttcp(void*, char*, int, int);
int	sprintospf(void*, char*, int);
int	sprintpup(void*, char*, int);
int	sprintarp(void*, char*);
int	sprintip(void*, char*, int);
int	sprintx(void*, char*, int);
int	sprintgre(void*, char*, int);
int	sprintesp(void*, char*, int);
int	sprintppp(void*, char*, int);
int	sprintlcpopt(void*, char*, int);
int	sprintipcpopt(void*, char*, int);
int	sprintccpopt(void*, char*, int);
int	sprintchap(void*, char*, int);
int	eaddrconv(void*, Fconv*);
int	ipaddrconv(void*, Fconv*);
char*	translate(NS*, char*);
ushort	ipcsum(uchar *addr);

void
usage(void)
{
	fprint(2, "usage: snoopy [-bpniaetulg9] [network-dir]\n");
	exits("usage");
}

void
error(char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	fprint(2, "snoopy: %s %s\n", buf, s);
	exits("death");
}

void
warning(char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	fprint(2, "snoopy: %s %s\n", buf, s);
}

ulong
gettime(void)
{
	char x[3*12+1];

	x[0] = 0;
	seek(timefd, 0, 0);
	read(timefd, x, 3*12);
	x[3*12] = 0;
	return strtoul(x+2*12, 0, 0);
}

void
main(int argc, char *argv[])
{
	Etherpkt e;
	long n, m;
	int fd, cfd, dfd;
	int all;
	char buf[8*1024];
	long start;
	char dev[256];
	char x[6];
	Dir d;
	char *cp;
	uchar h[6];
	long now;
	uvlong ts = 0, ots = 0;

	Binit(&bout, 1, OWRITE);
	all = 1;
	dfd = -1;
	ARGBEGIN{
	case 's':
		special = 1;
		break;
	case 'b':
		all = 0;
		bflag++;
		break;
	case 'p':
		pflag = 0;
		break;
	case 'n':
		nflag = 1;
		break;
	case 'i':
		all = 0;
		iflag++;
		break;
	case 'F':
		all = 0;
		cp = ARGF();
		if(cp == 0)
			usage();
		parseip(ipfilter, cp);
		break;
	case 'a':
		all = 0;
		aflag++;
		break;
	case 'c':
		all = 0;
		cflag++;
		break;
	case 'd':
		dfd = create("/tmp/snoopydump", OWRITE, 0664);
		break;
	case 'e':
		all = 0;
		eflag++;
		break;
	case 'E':
		all = 0;
		Eflag++;
		break;
	case 'g':
		all = 0;
		gflag++;
		break;
	case 't':
		all = 0;
		tflag++;
		break;
	case 'u':
		all = 0;
		uflag++;
		break;
	case 'r':
		all = 0;
		rflag++;
		break;
	case 'l':
		all = 0;
		lflag++;
		break;
	case 'x':
		xflag++;
		break;
	case 'N':
		cp = ARGF();
		if(cp == 0)
			usage();
		dumpl = atoi(cp);
		break;
	case '9':
		flag9++;
		fmtinstall('F', fcallconv);
		break;
	case 'o':
		all = 0;
		oflag++;
		break;
	case 'S':
		fromsniffer = 1;
		break;
	default:
		usage();
	}ARGEND
	eflag |= all;

	fmtinstall('E', eipconv);
	fmtinstall('V', eipconv);
	fmtinstall('X', fcallconv);

	if(fromsniffer){
		fd = 0;
	} else {
		if(argc > 0)
			strncpy(dev, argv[0], sizeof(dev));
		else
			strcpy(dev, "/net/ether0");
	
		snprint(buf, sizeof(buf), "%s!-1", dev);
		fd = dial(buf, 0, 0, &cfd);
		if(fd < 0)
			error("opening ether data");
		if(pflag && write(cfd, "promiscuous", sizeof("promiscuous")-1) <= 0)
			error("connecting");
		if(special && write(cfd, "headersonly", sizeof("headersonly")-1) <= 0)
			error("headersonly");
	}

	timefd = open("/dev/cputime", OREAD);
	start = gettime();

	if(special){
		for(;;){
			dirfstat(fd, &d);
			if(d.length == 0)
				break;
			read(fd, &e, sizeof(e));
		}
		print("Hit return to start analyzing:");
		read(0, &e, sizeof(e));
	}

	for(;;){
		if(fromsniffer){
			m = read(fd, h, 6);
			if(m < 6)
				break;
			m = (h[0]<<8)|h[1];
			ts = (h[2]<<24)|(h[3]<<16)|(h[4]<<8)|h[5];
			if(m <= 0 || m > sizeof(e))
				continue;
			m = read(fd, &e, m);
		} else
			m = read(fd, &e, sizeof(e));
		if(m <= 0)
			break;
		if(special)
			m -= 4;
		punt = 0;
		n = sprintpup(&e, buf, m);
		if(n>0 && punt==0){
			now = gettime()-start;
			if(dfd > -1){
				x[0] = m>>8;
				x[1] = m;
				x[2] = now>>24;
				x[3] = now>>16;
				x[4] = now>>8;
				x[5] = now;
				write(dfd, x, 6);
				write(dfd, &e, m);
			}
			if(special){
				ts = e.d[60];
				ts = (ts<<8) | e.d[61];
				ts = (ts<<8) | e.d[62];
				ts = (ts<<8) | e.d[63];
				ts = (ts<<8) | e.d[64];
				ts = (ts<<8) | e.d[65];
				ts = (ts<<8) | e.d[66];
				ts = (ts<<8) | e.d[67];
				Bprint(&bout, "%llud %llud ", ts, ts-ots);
				ots = ts;
			} else if(fromsniffer) {
				Bprint(&bout, "%llud ", ts);
			} else
				Bprint(&bout, "%lud ", gettime()-start);
			strcat(buf, "\n");
			Bwrite(&bout, buf, n+1);
		}
		Bflush(&bout);
	}
}

int
sprintpup(void *a, char *buf, int len)
{
	Etherpkt *p=a;
	int n=0;
	int t;

	t = NetS(p->type);
	len -= ETHERHDRSIZE;
	if((t==ET_ARP || t==ET_RARP) && aflag){
		n += sprint(buf+n, "%.4ux(%.4d %E > %E)", t, len, p->s, p->d);
		n += sprintarp(p, buf+n);
	} else if((t==0x0800||(t&0xFF00)==0x1000) && (cflag||iflag||rflag||uflag||tflag||bflag||lflag||gflag||oflag||Eflag)){
		if(eflag)
			n += sprint(buf+n, "%.4ux(%.4d %E > %E)", t, len, p->s, p->d);
		n += sprintip(p->data, buf+n, len);
	} else if(eflag){
		n += sprint(buf+n, "%.4ux(%.4d %E > %E)", t, len, p->s, p->d);
		n += sprintx(p->data, buf+n, len);
	} else
		punt = 1;
	return n;
}

/*
 *  arp packet body
 */
char *arpop[] = {
	"arp request",
	"arp reply",
	"rarp request",
	"rarp reply",
};

int
sprintarp(void *a, char *buf)
{
	int n = 0;
	Arppkt *ah = a;
	ushort op;

	op = NetS(ah->op);
	if(op < 5 && op > 0)
		n += sprint(buf, "ARP %s %d %d %d %d source %E %V target %E %V ", arpop[op-1],
			NetS(ah->hrd), NetS(ah->pro), ah->hln, ah->pln, ah->sha, ah->spa,
			ah->tha, ah->tpa);
	else
		n += sprint(buf, "%d %d %d %d %d source %E %V target %E %V ", op,
			NetS(ah->hrd), NetS(ah->pro), ah->hln, ah->pln, ah->sha, ah->spa,
			ah->tha, ah->tpa);
	return n;
}

int
sprintbootp(void *a, char *buf, int len)
{
	Bootp *boot = a;
	int n = 0;

	n += sprint(buf+n, "BOOTP(o %d t %d l %d h %d f %d id %lux s %d c %V y %V s %V g %V c %E %s %s) ",
		boot->op, boot->htype, boot->hlen, boot->hops,
		NetS(boot->flag),
		(ulong) NetL(boot->xid), NetS(boot->secs), boot->ciaddr,
		boot->yiaddr, boot->siaddr, boot->giaddr,
		boot->chaddr, boot->sname, boot->file);
	len -= 236;
	n += sprintx(boot->vend, buf+n, len);
	return n;
}

typedef struct Ilpkt	Ilpkt;
struct Ilpkt
{
	uchar	ilsum[2];	/* Checksum including header */
	uchar	illen[2];	/* Packet length */
	uchar	iltype;		/* Packet type */
	uchar	ilspec;		/* Special */
	uchar	ilsrc[2];	/* Src port */
	uchar	ildst[2];	/* Dst port */
	uchar	ilid[4];	/* Sequence id */
	uchar	ilack[4];	/* Acked sequence */
	char	data[1];
};
#define IL_HDRSIZE	18	

char *Iltype[] = { "sync", "data", "dataquery", "ack", "query", "state", "close" };

int
sprintil(void *a, char *buf, int len)
{
	Ilpkt *il = a;
	int i, n = 0;
	char *s, tbuf[10];
	Fcall f;

	len -= IL_HDRSIZE;

	s = tbuf;
	if(il->iltype < 0 || il->iltype > 6)
		sprint(tbuf, "BAD %d", il->iltype);
	else
		s = Iltype[il->iltype];

	n += sprint(buf, "%d IL(%d > %d c %.4x l %d %s %ud %ud %d) ", len,
		NetS(il->ilsrc), NetS(il->ildst), NetS(il->ilsum),
		NetS(il->illen), s, NetL(il->ilid), NetL(il->ilack),
		il->ilspec);

	if(flag9){
		/*
		 * Do the formatting regardless as this may be a first fragment
		 * in which case convM2S will fail the final length check but
		 * otherwise be ok.
		 * If %F can't format it (returns "unknown type %d"), don't print.
		 */
		convM2S(il->data, &f, len);
		i = sprint(buf+n, "%F", &f);
		if(buf[n] != 'u')
			n += i;
		else
			buf[n] = 0;
	} else
		n += sprintx(il->data, buf+n, len);

	return n;
}

typedef struct Icmppkt	Icmppkt;
struct Icmppkt
{
	uchar	type;
	uchar	code;
	uchar	cksum[2];	/* Checksum */
	char	data[1];
};
#define ICMP_HDRSIZE	4

char *icmpmsg[] =
{
[0]	"echo reply",
[3]	"destination unreachable",
[4]	"source quench",
[5]	"redirect",
[8]	"echo request",
[11]	"time exceeded",
[12]	"parameter problem",
[13]	"timestamp",
[14]	"timestamp reply",
[15]	"info request",
[16]	"info reply",
[255]	0,
};

int
sprinticmp(void *a, char *buf, int len)
{
	Icmppkt *icmp = a;
	int n = 0;
	char *t, *p;
	char tbuf[32];

	len -= ICMP_HDRSIZE;
	t = icmpmsg[icmp->type];
	if(t == 0){
		t = tbuf;
		sprint(tbuf, "type %d", icmp->type);
	}
	n += sprint(buf, "ICMP(%s code %d ck %.4ux) ", 
		t, icmp->code, NetS(icmp->cksum));
	p = icmp->data;
	if(icmp->type == 5){
		n += sprint(buf+n, "%p ", p);
		p += 4;
		len -= 4;
		n += sprintip(p, buf+n, len);
	} else
		n += sprintx(p, buf+n, len);
	return n;
}

typedef struct Udppkt	Udppkt;
struct Udppkt
{
	uchar	sport[2];	/* Source port */
	uchar	dport[2];	/* Destination port */
	uchar	len[2];		/* data length */
	uchar	cksum[2];	/* Checksum */
	char	data[1];
};
#define UDP_HDRSIZE	8

int
sprintudp(void *a, char *buf, int len)
{
	Udppkt *udp = a;
	int n = 0;

	len -= UDP_HDRSIZE;

	if((NetS(udp->dport)==67 || NetS(udp->dport)==68) && (bflag||rflag||uflag||iflag)){
		n += sprint(buf, "UDP(%d > %d ln %d ck %.4ux) ", 
			NetS(udp->sport), NetS(udp->dport),
			NetS(udp->len), NetS(udp->cksum));
		n += sprintbootp(udp->data, buf + n, len);
	} else if(rflag||uflag||iflag) {
		n += sprint(buf, "UDP(%d > %d ln %d ck %.4ux) ", 
			NetS(udp->sport), NetS(udp->dport),
			NetS(udp->len), NetS(udp->cksum));
		n += sprintx(udp->data, buf+n, len);
	} else
		punt = 1;
	return n;
}

typedef struct Rudppkt Rudppkt;
struct Rudppkt
{
	/* udp header */
	uchar	sport[2];	/* Source port */
	uchar	dport[2];	/* Destination port */
	uchar	len[2];		/* data length (includes rudp header) */
	uchar	cksum[2];	/* Checksum */

	/* rudp header */
	uchar	seq[4];		/* id of this packet (or 0) */
	uchar	sgen[4];	/* generation/time stamp */
	uchar	ack[4];		/* packet being acked (or 0) */
	uchar	agen[4];	/* generation/time stamp */

	char	data[1];
};
#define RUDP_HDRSIZE	24

int
sprintrudp(void *a, char *buf, int len)
{
	Rudppkt *rudp = a;
	int n = 0;

	len -= RUDP_HDRSIZE;

	if(rflag||uflag||iflag) {
		n += sprint(buf, "UDP(%d > %d ln %d ck %.4ux) ", 
			NetS(rudp->sport), NetS(rudp->dport),
			NetS(rudp->len), NetS(rudp->cksum));
		n += sprint(buf+n, "RUDP(%ud.%ux %ud.%ux) ", 
			NetL(rudp->seq), NetL(rudp->sgen),
			NetL(rudp->ack), NetL(rudp->agen));
		n += sprintx(rudp->data, buf+n, len);
	} else
		punt = 1;
	return n;
}

enum
{
	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Aknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */
};

char*
tcpflag(ushort flag)
{
	static char buf[128];

	sprint(buf, "%d", flag>>10);	/* Head len */
	if(flag & URG)
		strcat(buf, " URG");
	if(flag & ACK)
		strcat(buf, " ACK");
	if(flag & PSH)
		strcat(buf, " PSH");
	if(flag & RST)
		strcat(buf, " RST");
	if(flag & SYN)
		strcat(buf, " SYN");
	if(flag & FIN)
		strcat(buf, " FIN");

	return buf;
}

typedef struct Tcppkt	Tcppkt;
struct Tcppkt
{
	uchar	sport[2];
	uchar	dport[2];
	uchar	seq[4];
	uchar	ack[4];
	uchar	flag[2];
	uchar	win[2];
	uchar	cksum[2];
	uchar	urg[2];
	char	data[1];
};
#define TCP_HDRSIZE	20

int
sprinttcp(void *a, char *buf, int len, int iphdrlen)
{
	Tcppkt *tcp = a;
	int n = 0, hdrlen;
	Fcall f;

	hdrlen = (tcp->flag[0] & 0xf0)>>2;
	len -= iphdrlen;
	n += sprint(buf+n, "TCP(%d > %d s %8.8ux a %8.8ux %s w %.4ux l %d) ",
		NetS(tcp->sport), NetS(tcp->dport),
		NetL(tcp->seq), NetL(tcp->ack), tcpflag(NetS(tcp->flag)),
		NetS(tcp->win), len);
	len -= (hdrlen-iphdrlen);
	n += sprint(buf+n, "(len=%d)", len);

	if(flag9 && convM2S(tcp->data, &f, len))
		n += sprint(buf+n, "%F", &f);
	else
		n += sprintx(tcp->data, buf+n, len);

	return n;
}

/*
 *  OSPF packets
 */
typedef struct Ospfpkt	Ospfpkt;
struct Ospfpkt
{
	uchar	version;
	uchar	type;
	uchar	length[2];
	uchar	router[4];
	uchar	area[4];
	uchar	sum[2];
	uchar	autype[2];
	uchar	auth[8];
	uchar	data[1];
};
#define OSPF_HDRSIZE	24	

enum
{
	OSPFhello=	1,
	OSPFdd=		2,
	OSPFlsrequest=	3,
	OSPFlsupdate=	4,
	OSPFlsack=	5,
};


char *ospftype[] = {
	[OSPFhello]	"hello",
	[OSPFdd]	"data definition",
	[OSPFlsrequest]	"link state request",
	[OSPFlsupdate]	"link state update",
	[OSPFlsack]	"link state ack",
};

char*
ospfpkttype(int x)
{
	static char type[16];

	if(x > 0 && x <= OSPFlsack)
		return ospftype[x];
	sprint(type, "type %d", x);
	return type;
}

char*
ospfauth(Ospfpkt *ospf)
{
	static char auth[100];

	switch(ospf->type){
	case 0:
		return "no authentication";
	case 1:
		sprint(auth, "password(%8.8ux %8.8ux)", NetL(ospf->auth),	
			NetL(ospf->auth+4));
		break;
	case 2:
		sprint(auth, "crypto(plen %d id %d dlen %d)", NetS(ospf->auth),	
			ospf->auth[2], ospf->auth[3]);
		break;
	default:
		sprint(auth, "auth%d(%8.8ux %8.8ux)", NetS(ospf->autype), NetL(ospf->auth),	
			NetL(ospf->auth+4));
	}
	return auth;
}

typedef struct Ospfhello	Ospfhello;
struct Ospfhello
{
	uchar	mask[4];
	uchar	interval[2];
	uchar	options;
	uchar	pri;
	uchar	deadint[4];
	uchar	designated[4];
	uchar	bdesignated[4];
	uchar	neighbor[1];
};

int
sprintospfhello(void *p, char *buf)
{
	int n = 0;
	Ospfhello *h;

	h = p;

	n += sprint(buf+n, "%s(mask %V interval %d opt %ux pri %ux deadt %d designated %V bdesignated %V)",
		ospftype[OSPFhello],
		h->mask, NetS(h->interval), h->options, h->pri,
		NetL(h->deadint), h->designated, h->bdesignated);
	return n;
}

enum
{
	LSARouter=	1,
	LSANetwork=	2,
	LSASummN=	3,
	LSASummR=	4,
	LSAASext=	5
};


char *lsatype[] = {
	[LSARouter]	"Router LSA",
	[LSANetwork]	"Network LSA",
	[LSASummN]	"Summary LSA (Network)",
	[LSASummR]	"Summary LSA (Router)",
	[LSAASext]	"LSA AS external",
};

char*
lsapkttype(int x)
{
	static char type[16];

	if(x > 0 && x <= LSAASext)
		return lsatype[x];
	sprint(type, "type %d", x);
	return type;
}

/* OSPF Link State Advertisement Header */
/* rfc2178 section 12.1 */
/* data of Ospfpkt point to a 4-uchar value that is the # of LSAs */
struct OspfLSAhdr {
	uchar	lsage[2];
	uchar	options;	/* 0x2=stub area, 0x1=TOS routing capable */

	uchar	lstype;	/* 1=Router-LSAs
						 * 2=Network-LSAs
						 * 3=Summary-LSAs (to network)
						 * 4=Summary-LSAs (to AS boundary routers)
						 * 5=AS-External-LSAs
						 */
	uchar	lsid[4];
	uchar	advtrt[4];

	uchar	lsseqno[4];
	uchar	lscksum[2];
	uchar	lsalen[2];	/* includes the 20 byte lsa header */
};

struct Ospfrt {
	uchar	linkid[4];
	uchar	linkdata[4];
	uchar	typ;
	uchar	numtos;
	uchar	metric[2];
	
};

struct OspfrtLSA {
	struct OspfLSAhdr	hdr;
	uchar			netmask[4];
};

struct OspfntLSA {
	struct OspfLSAhdr	hdr;
	uchar			netmask[4];
	uchar			attrt[4];
};

/* Summary Link State Advertisement info */
struct Ospfsumm {
	uchar	flag;	/* always zero */
	uchar	metric[3];
};

struct OspfsummLSA {
	struct OspfLSAhdr	hdr;
	uchar			netmask[4];
	struct Ospfsumm		lsa;
};

/* AS external Link State Advertisement info */
struct OspfASext {
	uchar	flag;	/* external */
	uchar	metric[3];
	uchar	fwdaddr[4];
	uchar	exrttag[4];
};

struct OspfASextLSA {
	struct OspfLSAhdr	hdr;
	uchar			netmask[4];
	struct OspfASext	lsa;
};

/* OSPF Link State Update Packet */
struct OspfLSupdpkt {
	uchar	lsacnt[4];
	union {
		uchar			hdr[1];
		struct OspfrtLSA	rt[1];
		struct OspfntLSA	nt[1];
		struct OspfsummLSA	sum[1];
		struct OspfASextLSA	as[1];
	};
};

int
sprintospflsaheader(struct OspfLSAhdr *h, char *buf)
{
	int n = 0;

	n += sprint(buf+n, "age %d opt %ux type %ux lsid %V adv_rt %V seqno %ux c %4.4ux l %d",
		NetS(h->lsage), h->options&0xff, h->lstype,
		h->lsid, h->advtrt, NetL(h->lsseqno), NetS(h->lscksum),
		NetS(h->lsalen));
	return n;
}

/* OSPF Database Description Packet */
struct OspfDDpkt {
	uchar	intMTU[2];
	uchar	options;
	uchar	bits;
	uchar	DDseqno[4];
	struct OspfLSAhdr	hdr[1];		/* LSA headers... */
};

int
sprintospfdatadesc(void *p, char *buf, int len) {
	int n = 0, nlsa, i;
	struct OspfDDpkt *g;

	g = (struct OspfDDpkt *)p;
	nlsa = (len - OSPF_HDRSIZE)/sizeof(struct OspfLSAhdr);
	for (i=0; i<nlsa; i++) {
		n += sprint(buf+n, "lsa%d(", i);
		n += sprintospflsaheader(&(g->hdr[i]), buf+n);
		n += sprint(buf+n, ")");
	}
	n += sprint(buf+n, ")");
	return n;
}

int
sprintospflsupdate(void *p, char *buf)
{
	int n = 0, nlsa, i;
	struct OspfLSupdpkt *g;
	struct OspfLSAhdr *h;

	g = (struct OspfLSupdpkt *)p;
	nlsa = NetL(g->lsacnt);
	h = (struct OspfLSAhdr *)(g->hdr);
	n += sprint(buf+n, "%d-%s(", nlsa, ospfpkttype(OSPFlsupdate));

	switch(h->lstype) {
	case LSARouter:
		{
/*			struct OspfrtLSA *h;
 */
		}
		break;
	case LSANetwork:
		{
			struct OspfntLSA *h;

			for (i=0; i<nlsa; i++) {
				h = &(g->nt[i]);
				n += sprint(buf+n, "lsa%d(", i);
				n += sprintospflsaheader(&(h->hdr), buf+n);
				n += sprint(buf+n, " mask %V attrt %V)",
					h->netmask, h->attrt);
			}
		}
		break;
	case LSASummN:
	case LSASummR:
		{
			struct OspfsummLSA *h;

			for (i=0; i<nlsa; i++) {
				h = &(g->sum[i]);
				n += sprint(buf+n, "lsa%d(", i);
				n += sprintospflsaheader(&(h->hdr), buf+n);
				n += sprint(buf+n, " mask %V met %d)",
					h->netmask, Net3(h->lsa.metric));
			}
		}
		break;
	case LSAASext:
		{
			struct OspfASextLSA *h;

			for (i=0; i<nlsa; i++) {
				h = &(g->as[i]);
				n += sprint(buf+n, " lsa%d(", i);
				n += sprintospflsaheader(&(h->hdr), buf+n);
				n += sprint(buf+n, " mask %V extflg %1.1ux met %d fwdaddr %V extrtflg %ux)",
					h->netmask, h->lsa.flag, Net3(h->lsa.metric),
					h->lsa.fwdaddr, NetL(h->lsa.exrttag));
			}
		}
		break;
	default:
		n += sprint(buf+n, "Not an LS update, lstype %d ", h->lstype);
		n += sprintx(p, buf+n, 20);
		break;
	}
	n += sprint(buf+n, ")");
	return n;
}

int
sprintospflsack(void *p, char *buf, int len)
{
	int n = 0, nlsa, i;
	struct OspfLSAhdr *h;

	h = (struct OspfLSAhdr *)p;
	nlsa = (len - OSPF_HDRSIZE)/sizeof(struct OspfLSAhdr);
	n += sprint(buf+n, "%d-%s(", nlsa, ospfpkttype(OSPFlsack));
	for (i=0; i<nlsa; i++) {
		n += sprint(buf+n, " lsa%d(", i);
		n += sprintospflsaheader(&(h[i]), buf+n);
		n += sprint(buf+n, ")");
	}
	n += sprint(buf+n, ")");
	return n;
}

int
sprintospf(void *a, char *buf, int len)
{
	Ospfpkt *ospf = a;
	int n = 0;

	len -= OSPF_HDRSIZE;
	n += sprint(buf+n, "OSPFv%d(%d l %d r %V a %V c %4.4ux %s ",
		ospf->version, ospf->type, NetS(ospf->length),
		ospf->router, ospf->area, NetS(ospf->sum),
		ospfauth(ospf));
	switch (ospf->type) {
	case OSPFhello:
		n += sprintospfhello(ospf->data, buf+n);
		break;
	case OSPFdd:
		n += sprintospfdatadesc(ospf->data, buf+n, NetS(ospf->length));
		break;
	case OSPFlsrequest:
		n += sprint(buf+n, "%s->", ospfpkttype(ospf->type));
		goto Default;
	case OSPFlsupdate:
		n += sprintospflsupdate(ospf->data, buf+n);
		break;
	case OSPFlsack:
		n += sprintospflsack(ospf->data, buf+n, NetS(ospf->length));
		break;
	default:
Default:
		n += sprintx(ospf->data, buf+n, len);
	}
	n += sprint(buf+n, ")");
	return n;
}

int
sprintip(void *a, char *buf, int len)
{
	Ippkt *ip=a;
	int n = 0;
	int i, frag, id;
	int olen;

	len -= IP_HDRSIZE;
	olen = len;
	i = NetS(ip->length) - IP_HDRSIZE;
	id = NetS(ip->id);
	if(ipfilter[IPv4off])
	if(memcmp(ip->src, ipfilter+IPv4off, 4) != 0)
	if(memcmp(ip->dst, ipfilter+IPv4off, 4) != 0){
		punt = 1;
		return 0;
	}
	if(i >= 0 && i <= len)
		len = i;
	else if(!special)
		n += sprint(buf+n, "Bad IP len %d out of %d ", i, len);
	if(ipcsum((uchar*)ip) != 0)
		n += sprint(buf+n, "Bad IP chksum %ux ", ipcsum((uchar*)ip));
	if(ip->tos != 0)
		n += sprint(buf+n, "TOS(%ux)", ip->tos);
	if(ip->proto==IP_ICMPPROTO && (iflag||cflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "ICMP(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
			n += sprinticmp(ip->data, buf+n, len);
	} else if(ip->proto==IP_UDPPROTO && (iflag||uflag||bflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "UDP(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
	 		n += sprintudp(ip->data, buf+n, len);
	} else if(ip->proto==IP_RUDPPROTO && (iflag||rflag||bflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "RUDP(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
	 		n += sprintrudp(ip->data, buf+n, len);
	}else if(ip->proto==IP_TCPPROTO && (iflag||tflag)){
	  int iphdrlen = (ip->vihl&0xF)<<2;
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "TCP(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
			n += sprinttcp(ip->data, buf+n, len, iphdrlen);
	}else if(ip->proto==IP_OSPFPROTO && (iflag||oflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		n += sprintospf(ip->data, buf+n, len);
	}else if(ip->proto==IP_ILPROTO && (iflag||lflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "IL(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
			n += sprintil(ip->data, buf+n, len);	
	}else if(ip->proto==IP_GREPROTO && (iflag||gflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "GRE(FRAGMENT 0x%4.4ux len=%d/%d) ", frag, len, olen);
			n += sprintx(ip->data, buf+n, len);
		} else
			n += sprintgre(ip->data, buf+n, len);	
	}else if(ip->proto==IP_ESPPROTO && (iflag||Eflag)){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		frag = NetS(ip->frag);
		if(frag & ~(IP_MF|IP_DF)) {
			n += sprint(buf+n, "ESP(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		} else
			n += sprintesp(ip->data, buf+n, len);
	}else if(iflag){
		n += sprint(buf+n, "IP%d(%V > %V, %d/%d, %d) ", ip->proto, ip->src, ip->dst, ip->ttl, id, i);
		n += sprintx(ip->data, buf+n, len);
	} else
		punt = 1;
	return n;
}

typedef struct Esphdr {
	/* esp header */
	uchar	espspi[4];	/* Security parameter index */
	uchar	espseq[4];	/* Sequence number */
} Esphdr;


int
sprintesp(void *a, char *buf, int len)
{
	uchar *p = a;
	Esphdr *eh = a;
	int n = 0;

	n += sprint(buf+n, "ESP(spi=%ud seq=%ud) ",
		NetL(eh->espspi), NetL(eh->espseq));
	p += sizeof(Esphdr);
	len -= sizeof(Esphdr);
	n += sprintx(p, buf+n, len);

	return n;
}

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
	GRE_sna		= 0x0004,
	GRE_osi		= 0x00fe,
	GRE_pup		= 0x0200,
	GRE_xns		= 0x0600,
	GRE_ip		= 0x0800,
	GRE_chaos	= 0x0804,
	GRE_rfc826	= 0x0806,
	GRE_frarp	= 0x0808,
	GRE_vines	= 0x0bad,
	GRE_vinesecho	= 0x0bae,
	GRE_vinesloop	= 0x0baf,
	GRE_decnetIV	= 0x6003,
	GRE_ppp		= 0x880b,
};

int
sprintgre(void *a, char *buf, int len)
{
	int flag, prot, chksum, offset, key, seq, ack;
	int n;
	uchar *p = a;

	chksum = offset = key = seq = ack = 0;
	
	flag = NetS(p);
	prot = NetS(p+2);
	p += 4; len -= 4;
	if(flag & (GRE_chksum|GRE_routing)){
		chksum = NetS(p);
		offset = NetS(p+2);
		p += 4; len -= 4;
	}
	if(flag&GRE_key){
		key = NetL(p);
		p += 4; len -= 4;
	}
	if(flag&GRE_seq){
		seq = NetL(p);
		p += 4; len -= 4;
	}
	if(flag&GRE_ack){
		ack = NetL(p);
		p += 4; len -= 4;
	}
	/* skip routing if present */
	if(flag&GRE_routing) {
		while(len >= 4 && (n=p[3]) != 0) {
			len -= n;
			p += n;
		}
	}

	USED(offset);
	USED(chksum);

	n = sprint(buf, "GRE(f %4.4ux p %ux k %ux", flag, prot, key);
	if(flag&GRE_seq)
		n += sprint(buf+n, " s %ux", seq);
	if(flag&GRE_ack)
		n += sprint(buf+n, " a %ux", ack);
	n += sprint(buf+n, " len = %d/%d) ", len, key>>16);
	if(prot == GRE_ppp && len > 0)
		n += sprintppp(p, buf+n, len);
	else
		n += sprintx(p, buf+n, len);
		
	return n;
}

/* PPP stuff */
enum {
	PPP_addr=	0xff,
	PPP_ctl=	0x3,
	PPP_period=	3*1000,	/* period of retransmit process (in ms) */
};

/* PPP protocols */
enum {
	PPP_ip=		0x21,		/* internet */
	PPP_vjctcp=	0x2d,		/* compressing van jacobson tcp */
	PPP_vjutcp=	0x2f,		/* uncompressing van jacobson tcp */
	PPP_ml=		0x3d,		/* multi link */
	PPP_comp=	0xfd,		/* compressed packets */
	PPP_ipcp=	0x8021,		/* ip control */
	PPP_ccp=	0x80fd,		/* compression control */
	PPP_passwd=	0xc023,		/* passwd authentication */
	PPP_lcp=	0xc021,		/* link control */
	PPP_lqm=	0xc025,		/* link quality monitoring */
	PPP_chap=	0xc223,		/* challenge/response */
};

/* LCP protocol (and IPCP) */


typedef struct Lcppkt	Lcppkt;
struct Lcppkt
{
	uchar	code;
	uchar	id;
	uchar	len[2];
	uchar	data[1];
};

typedef struct Lcpopt	Lcpopt;
struct Lcpopt
{
	uchar	type;
	uchar	len;
	uchar	data[1];
};

enum
{
	/* LCP codes */
	Lconfreq=	1,
	Lconfack=	2,
	Lconfnak=	3,
	Lconfrej=	4,
	Ltermreq=	5,
	Ltermack=	6,
	Lcoderej=	7,
	Lprotorej=	8,
	Lechoreq=	9,
	Lechoack=	10,
	Ldiscard=	11,
	Lresetreq=	14,	/* for ccp only */
	Lresetack=	15,	/* for ccp only */

	/* Lcp configure options */
	Omtu=		1,
	Octlmap=	2,
	Oauth=		3,
	Oquality=	4,
	Omagic=		5,
	Opc=		7,
	Oac=		8,

	/* authentication protocols */
	APmd5=		5,
	APmschap=	128,

	/* Chap codes */
	Cchallenge=	1,
	Cresponse=	2,
	Csuccess=	3,
	Cfailure=	4,

	/* ipcp configure options */
	Oipaddrs=	1,
	Oipcompress=	2,
	Oipaddr=	3,
	Oipdns=		129,
	Oipwins=	130,
	Oipdns2=	131,
	Oipwins2=	132,
};

char *
lcpcode[] = {
	0,
	"confreq",
	"confack",
	"confnak",
	"confrej",
	"termreq",
	"termack",
	"coderej",
	"protorej",
	"echoreq",
	"echoack",
	"discard",
	"id",
	"timeremain",
	"resetreq",
	"resetack",
};

int
sprintppp(void *a, char *buf, int len)
{
	int i, n;
	uchar *p = a;
	ushort count;
	char compflag[5];
	int prot, oiflag;
	Lcppkt *lcp;

	if(p[0] == PPP_addr && p[1] == PPP_ctl) {
		p += 2;
		len -= 2;
	}

	prot = *p++; len--;

	if((prot&1) == 0) {
		prot = (prot<<8) | *p++;
		len--;
	}
	
	n = sprint(buf, "PPP(p %ux) ", prot);

	switch(prot) {
	default:
		n += sprintx(p, buf+n, len);
		break;
	case PPP_ip:
		oiflag = iflag;
		iflag = 1;
		n += sprintip(p, buf+n, len);
		iflag = oiflag;
		break;
	case PPP_ml:
		n += sprint(buf+n, "ML() ");
		n += sprintx(p, buf+n, len);
		break;
	case PPP_lcp:
		lcp = (Lcppkt*)p;
		n += sprint(buf+n, "LCP(id=%d ", lcp->id);
		switch(lcp->code) {
		default:
			n += sprint(buf+n, "unknown code %d", lcp->code);
			break;
		case Lconfreq:
		case Lconfack:
		case Lconfnak:
		case Lconfrej:
			n += sprint(buf+n, "%s ", lcpcode[lcp->code]);
			n += sprintlcpopt(lcp->data, buf+n, NetS(lcp->len)-4);
			break;
		case Ltermreq:
		case Ltermack:
		case Lcoderej:
		case Lprotorej:
		case Lechoreq:
		case Lechoack:
		case Ldiscard:
			n += sprint(buf+n, "%s len=%d", lcpcode[lcp->code], NetS(lcp->len));
			break;
		}
		n += sprint(buf+n, ") ");
		break;
	case PPP_ipcp:
		lcp = (Lcppkt*)p;
		n += sprint(buf+n, "IPCP(id=%d ", lcp->id);
		switch(lcp->code) {
		default:
			n += sprint(buf+n, "unknown code %d", lcp->code);
			break;
		case Lconfreq:
		case Lconfack:
		case Lconfnak:
		case Lconfrej:
			n += sprint(buf+n, "%s ", lcpcode[lcp->code]);
			n += sprintipcpopt(lcp->data, buf+n, NetS(lcp->len)-4);
			break;
		case Ltermreq:
		case Ltermack:
			n += sprint(buf+n, "%s len=%d", lcpcode[lcp->code], NetS(lcp->len));
			break;
		}
		n += sprint(buf+n, ") ");
		break;
	case PPP_ccp:
		lcp = (Lcppkt*)p;
		n += sprint(buf+n, "CCP(id=%d ", lcp->id);
		switch(lcp->code) {
		default:
			n += sprint(buf+n, "unknown code %d", lcp->code);
			break;
		case Lconfreq:
		case Lconfack:
		case Lconfnak:
		case Lconfrej:
			n += sprint(buf+n, "%s ", lcpcode[lcp->code]);
			n += sprintccpopt(lcp->data, buf+n, NetS(lcp->len)-4);
			break;
		case Ltermreq:
		case Ltermack:
		case Lresetreq:
		case Lresetack:
			n += sprint(buf+n, "%s len=%d", lcpcode[lcp->code], NetS(lcp->len));
			break;
		}
		n += sprint(buf+n, ") ");
		break;
	case PPP_chap:
		n += sprintchap(p, buf+n, len);
		break;
	case PPP_comp:
		if(len < 2) {
			n += sprint(buf+n, "COMP short! ");
			break;
		}
		count = NetS(p);
		i = 0;
		if(count & (1<<15))
			compflag[i++] = 'r';
		if(count & (1<<14))
			compflag[i++] = 'f';
		if(count & (1<<13))
			compflag[i++] = 'c';
		if(count & (1<<12))
			compflag[i++] = 'e';
		compflag[i] = 0;
		n += sprint(buf+n, "COMP(%s %.3ux) ", compflag, count&0xfff);
		n += sprintx(p+2, buf+n, len-2);
		break;
	}
	return n;
}

int
sprintlcpopt(void *a, char *buf, int len)
{
	Lcpopt *o;
	int proto, x, period;
	int n;
	uchar *cp, *ecp;

	cp = a;
	ecp = cp+len;
	n = 0;

	n += sprint(buf+n, " len=%d,", len);

	for(; cp < ecp; cp += o->len){
		o = (Lcpopt*)cp;
		if(cp + o->len > ecp || o->len == 0){
			n += sprint(buf+n, " bad opt len %d", o->len);
			return n;
		}

		switch(o->type){
		default:
			n += sprint(buf+n, " unknown %d len=%d,", o->type, o->len);
			break;
		case Omtu:
			n += sprint(buf+n, " mtu = %d,", NetS(o->data));
			break;
		case Octlmap:
			n += sprint(buf+n, " ctlmap = %ux,", NetL(o->data));
			break;
		case Oauth:
			n += sprint(buf+n, " auth = %ud", NetL(o->data));
			proto = NetS(o->data);
			switch(proto) {
			default:
				n += sprint(buf+n, "unknown auth proto %d,", proto);
				break;
			case PPP_passwd:
				n += sprint(buf+n, "passwd,");
				break;
			case PPP_chap:
				n += sprint(buf+n, "chap %ux,", o->data[2]);
				break;
			}
			break;
		case Oquality:
			proto = NetS(o->data);
			switch(proto) {
			default:
				n += sprint(buf+n, " unknown quality proto %d,", proto);
				break;
			case PPP_lqm:
				x = NetL(o->data+2)*10;
				period = (x+(PPP_period-1))/PPP_period;
				n += sprint(buf+n, " lqm period = %d,", period);
				break;
			}
		case Omagic:
			n += sprint(buf+n, " magic = %ux,", NetL(o->data));
			break;
		case Opc:
			n += sprint(buf+n, " protocol compress,");
			break;
		case Oac:
			n += sprint(buf+n, " addr compress,");
			break;
		}
	}
	return n;
}

int
sprintipcpopt(void *a, char *buf, int len)
{
	Lcpopt *o;
	int n;
	uchar *cp, *ecp;

	cp = a;
	ecp = cp+len;
	n = 0;

	for(; cp < ecp; cp += o->len){
		o = (Lcpopt*)cp;
		if(cp + o->len > ecp){
			n += sprint(buf+n, " bad opt len %ux", o->type);
			return n;
		}

		switch(o->type){
		default:
			n += sprint(buf+n, " unknown %d len=%d,", o->type, o->len);
			break;
		case Oipaddrs:	
			n += sprint(buf+n, " ip addrs - deprecated,");
			break;
		case Oipcompress:
			n += sprint(buf+n,  " ip compress,");
			break;
		case Oipaddr:	
			n += sprint(buf+n, " ip addr %V,", o->data);
			break;
		case Oipdns:	
			n += sprint(buf+n, " dns addr %V\n", o->data);
			break;
		case Oipwins:	
			n += sprint(buf+n, " wins addr %V\n", o->data);
			break;
		case Oipdns2:	
			n += sprint(buf+n, " dns2 addr %V\n", o->data);
			break;
		case Oipwins2:	
			n += sprint(buf+n, " wins2 addr %V\n", o->data);
			break;
		}
	}
	return n;
}

int
sprintccpopt(void *a, char *buf, int len)
{
	Lcpopt *o;
	int n;
	uchar *cp, *ecp;

	cp = a;
	ecp = cp+len;
	n = 0;

	for(; cp < ecp; cp += o->len){
		o = (Lcpopt*)cp;
		if(cp + o->len > ecp){
			n += sprint(buf+n, " bad opt len %ux", o->type);
			return n;
		}
		
		n += sprint(buf+n, "compress=");

		switch(o->type){
		default:
			n += sprint(buf+n, "type %d ", o->type);
			break;
		case 0:
			n += sprint(buf+n, "OUI %d %.2ux%.2ux%.2ux ", o->type, 
				o->data[0], o->data[1], o->data[2]);
			break;
		case 17:
			n += sprint(buf+n, "Stac LZS ");
			break;
		case 18:
			n += sprint(buf+n, "Microsoft PPC opt=%ux ", NetL(o->data));
			break;
		}
	}
	return n;
}

int
sprintchap(void *a, char *buf, int len)
{
	Lcppkt *p;
	int n;

	p = a;
	n = 0;

	if(len < NetS(p->len)) {
		n += sprint(buf+n, "CHAP(short packet)");
		return n;
	}

	len = NetS(p->len);
		
if(p->code == 6)
write(2, p, len);
	
	n += sprint(buf+n, "CHAP(id=%d ", p->id);
	switch(p->code) {
	default:
		n += sprint(buf+n, "code=%d len=%d", p->code, len);
		n += sprintx(p->data, buf+n, len-4);
		break;
	case 1:
	case 2:
		n += sprint(buf+n, "%s", p->code==1?"challenge ":"responce ");
		n += sprintx(p->data+1, buf+n, p->data[0]);
		n += sprint(buf+n, " name ");
		n += sprintx(p->data+p->data[0]+1, buf+n, len-4-p->data[0]-1);
		break;
	case 3:
	case 4:
		n += sprint(buf+n, "%s", p->code==3?"success ":"failure");
		n += sprintx(p->data, buf+n, len-4);
		break;
	}
	n += sprint(buf+n, ")");
	return n;
}

char *
translate(NS *nsp, char *name)
{
	Ndbtuple *t;
	int i;

	/*
	 *  look for name in cache
	 */
	for(i = 0; i < Ncache; i++){
		if(strcmp(name, nsp->cache[i].before)==0){
			return nsp->cache[i].after;
		}
	}
	i = nsp->n;
	nsp->n = (i+1)%Ncache;
	strcpy(nsp->cache[i].before, name);
	t = csgetval(netroot, nsp->attr, name, "sys", nsp->cache[i].after);
	if(t)
		ndbfree(t);
	if(nsp->cache[i].after[0] == 0)
		strcpy(nsp->cache[i].after, name);

	return nsp->cache[i].after;
}

int
sprintx(void *f, char *to, int count)
{
	int i, printable;
	char *start = to;
	uchar *from = f;

	if(count < 0) {
		print("BAD DATA COUNT %d\n", count);
		return 0;
	}
	printable = !xflag;
	if(count > dumpl)
		count = dumpl;
	for(i=0; i<count && printable; i++)
		if(!isspace(from[i]) && !isprint(from[i]))
			printable = 0;
	*to++ = '\'';
	if(printable){
		memmove(to, from, count);
		to += count;
	}else{
		for(i=0; i<count; i++){
			if(i>0 && i%4==0)
				*to++ = ' ';
			sprint(to, "%2.2ux", from[i]);
			to += 2;
		}
	}
	*to++ = '\'';
	*to = 0;
	return to - start;
}

ushort
ipcsum(uchar *addr)
{
	int len;
	ulong sum;

	sum = 0;
	len = (addr[0]&0xf)<<2;

	while(len > 0) {
		sum += addr[0]<<8 | addr[1] ;
		len -= 2;
		addr += 2;
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return (sum^0xffff);
}
