typedef	struct	Enpkt	Enpkt;
typedef	struct	Arppkt	Arppkt;
typedef	struct	Ippkt	Ippkt;
typedef	struct	Ilpkt	Ilpkt;
typedef	struct	Udppkt	Udppkt;
typedef	struct	Icmppkt	Icmppkt;
typedef	struct	Ifc	Ifc;

enum
{
	Easize		= 6,		/* Ether address size */
	Pasize		= 4,		/* IP protocol address size */
};

enum
{
	Nqt=	8,
};

typedef
struct	Ilp
{
	Queue*	reply;		/* ethernet output */
	uchar	iphis[Pasize];	/* his ip address (index) */
	uchar	ipgate[Pasize];	/* his ip/gateway address */
	Chan*	chan;		/* list of il channels */

	int	alloc;		/* 1 means allocated */
	int	srcp;		/* source port (index) */
	int	dstp;		/* dest port (index) */
	int	state;		/* connection state */

	Lock;

	Msgbuf*	unacked;
	Msgbuf*	unackedtail;

	Msgbuf*	outoforder;

	ulong	next;		/* id of next to send */
	ulong	recvd;		/* last packet received */
	ulong	start;		/* local start id */
	ulong	rstart;		/* remote start id */
	ulong	acksent;	/* Last packet acked */

	ulong	lastxmit;	/* time of last xmit */
	ulong	lastrecv;	/* time of last recv */
	ulong	timeout;	/* time out counter */
	ulong	acktime;	/* acknowledge timer */
	ulong	querytime;	/* Query timer */

	ulong	delay;		/* Average of the fixed rtt delay */
	ulong	rate;		/* Average byte rate */
	ulong	mdev;		/* Mean deviation of predicted to real rtt */
	ulong	maxrtt;		/* largest rtt seen */
	ulong	rttack;		/* The ack we are waiting for */
	int	rttlen;		/* Length of rttack packet */
	ulong	rttstart;	/* Time we issued rttack packet */
	ulong	unackedbytes;
	int	rexmit;		/* number of rexmits of *unacked */

	ulong	qt[Nqt+1];	/* state table for query messages */
	int	qtx;		/* ... index into qt */

	int	window;		/* maximum receive window */

	Rendez	syn;		/* connect hang out */
} Ilp;

/*
 * Ethernet header
 */
enum
{
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */

	Arptype		= 0x0806,
	Iptype		= 0x0800,

	Icmpproto	= 1,
	Igmpproto	= 2,
	Tcpproto	= 6,
	Udpproto	= 17,
	Ilproto		= 40,

	Nqueue		= 20,
	Nfrag		= 6,		/* max number of non-contig ip fragments */
	Nrock		= 20,		/* number of partial ip assembly stations */
	Nb		= 211,		/* number of arp hash buckets */
	Ne		= 10,		/* number of entries in each arp hash bucket */

	Ensize		= 14,		/* ether header size */
	Ipsize		= 20,		/* ip header size -- doesnt include Ensize */
	Arpsize		= 28,		/* arp header size -- doesnt include Ensize */
	Ilsize		= 18,		/* il header size -- doesnt include Ipsize/Ensize */
	Udpsize		= 8,		/* il header size -- doesnt include Ipsize/Ensize */
	Udpphsize	= 12,		/* udp pseudo ip header size */

	IP_VER		= 0x40,			/* Using IP version 4 */
	IP_HLEN		= Ipsize/4,		/* Header length in longs */
	IP_DF		= 0x4000,		/* Don't fragment */
	IP_MF		= 0x2000,		/* More fragments */

	Arprequest	= 1,
	Arpreply,

	Ilfsport	= 17008,
	Ilauthport	= 17020,
	Ilfsout		= 5000,
	SNTP		= 123,
	SNTP_LOCAL	= 6001,
};

struct	Enpkt
{
	uchar	d[Easize];		/* destination address */
	uchar	s[Easize];		/* source address */
	uchar	type[2];		/* packet type */

	uchar	data[ETHERMAXTU-(6+6+2)];
	uchar	crc[4];
};

struct	Arppkt
{
	uchar	d[Easize];			/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	hrd[2];				/* hardware type, must be ether==1 */
	uchar	pro[2];				/* protocol, must be ip */
	uchar	hln;				/* hardware address len, must be Easize */
	uchar	pln;				/* protocol address len, must be Pasize */
	uchar	op[2];
	uchar	sha[Easize];
	uchar	spa[Pasize];
	uchar	tha[Easize];
	uchar	tpa[Pasize];
};

struct	Ippkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* Version and header length */
	uchar	tos;			/* Type of service */
	uchar	length[2];		/* packet length */
	uchar	id[2];			/* Identification */
	uchar	frag[2];		/* Fragment information */
	uchar	ttl;			/* Time to live */
	uchar	proto;			/* Protocol */
	uchar	cksum[2];		/* Header checksum */
	uchar	src[Pasize];		/* Ip source */
	uchar	dst[Pasize];		/* Ip destination */
};

struct	Ilpkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	ilsum[2];		/* Checksum including header */
	uchar	illen[2];		/* Packet length */
	uchar	iltype;			/* Packet type */
	uchar	ilspec;			/* Special */
	uchar	ilsrc[2];		/* Src port */
	uchar	ildst[2];		/* Dst port */
	uchar	ilid[4];		/* Sequence id */
	uchar	ilack[4];		/* Acked sequence */
};

struct	Udppkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	udpsrc[2];		/* Src port */
	uchar	udpdst[2];		/* Dst port */
	uchar	udplen[2];		/* Packet length */
	uchar	udpsum[2];		/* Checksum including header */
};

struct	Icmppkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	icmptype;		/* Src port */
	uchar	icmpcode;		/* Dst port */
	uchar	icmpsum[2];		/* Checksum including header */

	uchar	icmpbody[10];		/* Depends on type */
};

struct	Ifc
{
	Lock;
	Queue*	reply;
	Filter	work[3];
	Filter	rate[3];
	ulong	rcverr;
	ulong	txerr;
	ulong	sumerr;
	ulong	rxpkt;
	ulong	txpkt;
	uchar	ea[Easize];		/* my ether address */
	uchar	ipa[Pasize];		/* my ip address, pulled from netdb */
	uchar	netgate[Pasize];	/* my ip gateway, pulled from netdb */
	ulong	ipaddr;
	ulong	mask;
	ulong	cmask;
	Ifc	*next;			/* List of configured interfaces */
};

Ifc*	enets;			/* List of configured interfaces */

void	riprecv(Msgbuf*, Ifc*);
void	sntprecv(Msgbuf *mb, Ifc *ifc);

void	arpreceive(Enpkt*, int, Ifc*);
void	ipreceive(Enpkt*, int, Ifc*);
void	ilrecv(Msgbuf*, Ifc*);
void	udprecv(Msgbuf*, Ifc*);
void	ilrecv(Msgbuf*, Ifc*);
void	icmprecv(Msgbuf*, Ifc*);
void	igmprecv(Msgbuf*, Ifc*);
void	tcprecv(Msgbuf*, Ifc*);
void	iprouteinit(void);
long	ipclassmask(uchar*);
void	iproute(uchar*, uchar*, uchar*);

void	getipa(Ifc*, int);
int	ipforme(uchar*, Ifc*);
int	ipcsum(uchar*);

int	ptclcsum(uchar*, int);
void	ipsend(Msgbuf*);
void	ipsend1(Msgbuf*, Ifc*, uchar*);

uchar 	authip[Pasize];		/* ip address of server - from config block */
uchar	sntpip[Pasize];		/* ip address of sntp server */
struct
{
	uchar	sysip[Pasize];	/* my ip - from config block */
	uchar	defmask[Pasize];/* ip mask - from config block */
	uchar	defgwip[Pasize];/* gateway ip - from config block */
} ipaddr[10];
