#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <ndb.h>

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
 *  arp packet body
 */
#include "arp.h"
char *arpop[] = {
	"arp request",
	"arp reply",
	"rarp request",
	"rarp reply",
};

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
#define IP_TCPPROTO	6
#define	IP_ILPROTO	40
#define	IP_ICMPPROTO	1
#define	IP_DF		0x4000
#define	IP_MF		0x2000

typedef struct Icmppkt	Icmppkt;
struct Icmppkt
{
	uchar	type;
	uchar	code;
	uchar	cksum[2];	/* Checksum */
	char	data[1];
};
#define ICMP_HDRSIZE	4

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

char *Iltype[] = { "sync", "data", "dataquerey", "ack", "querey", "state", "close" };

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

/*
 *  Bootp packet
 */
#include "bootp.h"

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
#define NetL(x) (((x)[0]<<24) | ((x)[1]<<16) | ((x)[2]<<8) | (x)[3])

/*
 *  run flags
 */
int eflag;
int pflag = 1;
int aflag;
int bflag;
int iflag;
int cflag;
int nflag;
int uflag;
int tflag;
int lflag;
int flag9;
int punt;
int timefd;
Biobuf bout;

#define DUMPL 20
int dumpl = DUMPL;

/*
 *  predeclared
 */
int	sprintbootp(void*, char*, int);
int	sprinticmp(void*, char*, int);
int	sprintudp(void*, char*, int);
int	sprintil(void*, char*, int);
int	sprinttcp(void*, char*, int);
int	sprintpup(void*, char*, int);
int	sprintarp(void*, char*);
int	sprintip(void*, char*, int);
int	sprintx(void*, char*, int);
int	eaddrconv(void*, Fconv*);
int	ipaddrconv(void*, Fconv*);
char*	translate(NS*, char*);

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
	long n;
	int fd, cfd;
	int all;
	char buf[16*1024];
	long start;

	Binit(&bout, 1, OWRITE);
	all = 1;
	ARGBEGIN{
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
	case 'a':
		all = 0;
		aflag++;
		break;
	case 'c':
		all = 0;
		cflag++;
		break;
	case 'e':
		all = 0;
		eflag++;
		break;
	case 't':
		all = 0;
		tflag++;
		break;
	case 'u':
		all = 0;
		uflag++;
		break;
	case 'l':
		all = 0;
		lflag++;
		break;
	case '9':
		flag9++;
		fmtinstall('F', fcallconv);
		break;
	case 'N':
		dumpl = atoi(ARGF());
		break;
	default:
		fprint(2, "usage: snoopy [-bpniaetul9]\n");
		exits("usage");
	}ARGEND
	eflag |= all;

	fmtinstall('E', eaddrconv);
	fmtinstall('I', ipaddrconv);
	fmtinstall('X', fcallconv);

	fd = dial("ether!-1", 0, 0, &cfd);
	if(fd < 0)
		error("opening ether data");
	if(pflag && write(cfd, "promiscuous", sizeof("promiscuous")-1) <= 0)
		error("connecting");

	timefd = open("/dev/cputime", OREAD);
	start = gettime();
	for(;;){
		n = read(fd, &e, sizeof(e));
		if(n <= 0)
			break;
		punt = 0;
		n = sprintpup(&e, buf, n);
		if(n>0 && punt==0){
			Bprint(&bout, "%d ", gettime()-start);
			strcat(buf, "\n");
			Bwrite(&bout, buf, n+1);
		}
		Bflush(&bout);
	}
}

int
eaddrconv(void *v, Fconv *f)
{
	uchar *ea;
	char buf[32];
	char *name;

	ea = *(uchar **)v;
	sprint(buf, "%.2ux%.2ux%.2ux%.2ux%.2ux%.2ux",
		ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);
	if(nflag)
		name = buf;
	else
		name = translate(&etns, buf);
	strconv(name, f);
	return(sizeof(char*));
}

int
ipaddrconv(void *v, Fconv *f)
{
	uchar *ip;
	char buf[32];
	char *name;

	ip = *(uchar **)v;
	sprint(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	if(nflag)
		name = buf;
	else
		name = translate(&ipns, buf);
	strconv(name, f);
	return(sizeof(char*));
}

int
sprintpup(void *a, char *buf, int len)
{
	Etherpkt *p=a;
	int n=0;
	int t;

	t = NetS(p->type);
	len -= ETHERHDRSIZE;
	if(t==0x0806 && aflag){
		if(eflag)
			n += sprint(buf+n, "%.4ux(%.4d %E > %E)", t, len, p->s, p->d);
		n += sprintarp(p, buf+n);
	} else if((t==0x0800||(t&0xFF00)==0x1000) && (cflag||iflag||uflag||tflag||bflag||lflag)){
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

int
sprintarp(void *a, char *buf)
{
	int n = 0;
	Arppkt *ah = a;
	ushort op;

	op = NetS(ah->op);
	if(op < 5 && op > 0)
		n += sprint(buf, "ARP %s %d %d %d %d source %E %I target %E %I\n", arpop[op-1],
			NetS(ah->hrd), NetS(ah->pro), ah->hln, ah->pln, ah->sha, ah->spa,
			ah->tha, ah->tpa);
	else
		n += sprint(buf, "%d %d %d %d %d source %E %I target %E %I\n", op,
			NetS(ah->hrd), NetS(ah->pro), ah->hln, ah->pln, ah->sha, ah->spa,
			ah->tha, ah->tpa);
	return n;
}

int
sprintbootp(void *a, char *buf, int len)
{
	Bootp *boot = a;
	int n = 0;

	n += sprint(buf+n, "BOOTP(%d %d %d %d %lux %d c %I y %I s %I g %I c %E %s %s) ",
		boot->op, boot->htype, boot->hlen, boot->hops,
		NetL(boot->xid), NetS(boot->secs), boot->ciaddr,
		boot->yiaddr, boot->siaddr, boot->giaddr,
		boot->chaddr, boot->sname, boot->file);
	len -= 136;
	n += sprintx(boot->vend, buf+n, len);
	return n;
}

int
sprintil(void *a, char *buf, int len)
{
	Ilpkt *il = a;
	int n = 0, ok;
	char *s, tbuf[10];
	Fcall f;

	len -= IL_HDRSIZE;

	s = tbuf;
	if(il->iltype < 0 || il->iltype > 6)
		sprint(tbuf, "BAD %d", il->iltype);
	else
		s = Iltype[il->iltype];

	n += sprint(buf, "%d IL(%d > %d c %.4x l %d %s %lud %lud) ", len,
		NetS(il->ilsrc), NetS(il->ildst), NetS(il->ilsum),
		NetS(il->illen), s, NetL(il->ilid), NetL(il->ilack));

	if(flag9) {
		memset(&f, 0, sizeof(Fcall));
		ok = convM2S(il->data, &f, len);
		n += sprint(buf+n, "ok %d %F", ok, &f);
	} else
		n += sprintx(il->data, buf+n, len);

	return n;
}

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
	char *t;
	char tbuf[32];

	len -= ICMP_HDRSIZE;
	t = icmpmsg[icmp->type];
	if(t == 0){
		t = tbuf;
		sprint(tbuf, "type %d", icmp->type);
	}
	n += sprint(buf, "ICMP(%s code %d ck %.4ux) ", 
		t, icmp->code, NetS(icmp->cksum));
	n += sprintx(icmp->data, buf+n, len);
	return n;
}

int
sprintudp(void *a, char *buf, int len)
{
	Udppkt *udp = a;
	int n = 0;

	len -= UDP_HDRSIZE;

	if((NetS(udp->dport)==67 || NetS(udp->dport)==68) && (bflag||uflag|iflag)){
		n += sprint(buf, "UDP(%d > %d ln %d ck %.4ux) ", 
			NetS(udp->sport), NetS(udp->dport),
			NetS(udp->len), NetS(udp->cksum));
		n += sprintbootp(udp->data, buf + n, len);
	} else if(uflag||iflag) {
		n += sprint(buf, "UDP(%d > %d ln %d ck %.4ux) ", 
			NetS(udp->sport), NetS(udp->dport),
			NetS(udp->len), NetS(udp->cksum));
		n += sprintx(udp->data, buf+n, len);
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

int
sprinttcp(void *a, char *buf, int len)
{
	Tcppkt *tcp = a;
	int ok, n = 0;
	Fcall f;

	len -= TCP_HDRSIZE;
	n += sprint(buf+n, "TCP(%d > %d s %l8.8ux a %8.8lux %s w %.4ux l %d) ",
		NetS(tcp->sport), NetS(tcp->dport),
		NetL(tcp->seq), NetL(tcp->ack), tcpflag(NetS(tcp->flag)),
		NetS(tcp->win), len);

	if(len <= 0)
		n += sprint(buf+n, "(len=%d)\n", len);
	else
	if(flag9) {
		memset(&f, 0, sizeof(Fcall));
		ok = convM2S(tcp->data, &f, len);
		n += sprint(buf+n, "ok %d %F", ok, &f);
	}
	else
		n += sprintx(tcp->data, buf+n, len);

	return n;
}

int
sprintip(void *a, char *buf, int len)
{
	Ippkt *ip=a;
	int n = 0;
	int i, frag, id;

	len -= IP_HDRSIZE;
	i = NetS(ip->length) - IP_HDRSIZE;
	if(i >= 0 && i <= len)
		len = i;
	else
		n += sprint(buf+n, "Bad IP len %d ", i);
	id = NetS(ip->id);
	if(ip->proto==IP_ICMPPROTO && (iflag||cflag)){
		n += sprint(buf+n, "IP%d(%I > %I) %2d ", ip->proto, ip->src, ip->dst, id);
		n += sprinticmp(ip->data, buf+n, len);
	} else if(ip->proto==IP_UDPPROTO && (iflag||uflag||bflag)){
		n += sprint(buf+n, "IP%d(%I > %I) %2d ", ip->proto, ip->src, ip->dst, id);
		n += sprintudp(ip->data, buf+n, len);
	}else if(ip->proto==IP_TCPPROTO && (iflag||tflag)){
		n += sprint(buf+n, "IP%d(%I > %I) %2d ", ip->proto, ip->src, ip->dst, id);
		n += sprinttcp(ip->data, buf+n, len);
	}else if(ip->proto==IP_ILPROTO && (iflag||lflag)){
		n += sprint(buf+n, "IP%d(%I > %I) %2d ", ip->proto, ip->src, ip->dst, id);
		frag = NetS(ip->frag);
		if(frag & ~(IP_DF|IP_MF)) {
			n += sprint(buf+n, "IL(FRAGMENT 0x%4.4ux) ", frag);
			n += sprintx(ip->data, buf+n, len);
		}
		else
			n += sprintil(ip->data, buf+n, len);	
	}else if(iflag){
		n += sprint(buf+n, "IP%d(%I > %I) ", ip->proto, ip->src, ip->dst);
		n += sprintx(ip->data, buf+n, len);
	} else
		punt = 1;
	return n;
}

int
mygetfields(char *lp, char **fields, int n, char sep)
{
	int i;
	char sep2=0;

	if(sep == ' ')
		sep2 = '\t';
	for(i=0; lp && *lp && i<n; i++){
		while(*lp==sep || *lp==sep2)
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp!=sep && *lp!=sep2)
			lp++;
	}
	return i;
}

char *
translate(NS *nsp, char *name)
{
	static Ndb *db;
	Ndbtuple *t;
	Ndbs s;
	int i;

	/*
	 *  look for name in cache
	 */
	for(i = 0; i < Ncache; i++){
		if(strcmp(name, nsp->cache[i].before)==0){
			return nsp->cache[i].after;
		}
	}

	if(db == 0)
		db = ndbopen(0);

	i = nsp->n;
	nsp->n = (i+1)%Ncache;
	strcpy(nsp->cache[i].before, name);
	strcpy(nsp->cache[i].after, name);
	if(db){
		t = ndbgetval(db, &s, nsp->attr, name, "sys", nsp->cache[i].after);
		if(t)
			ndbfree(t);
	}

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
	printable = 1;
	if(count > dumpl)
		count = dumpl;
	for(i=0; i<count && printable; i++)
		if((from[i]<32 && from[i] !='\n' && from[i] !='\r' && from[i] !='\b' && from[i] !='\t') || from[i]>127)
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
