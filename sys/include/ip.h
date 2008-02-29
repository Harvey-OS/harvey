#pragma	src	"/sys/src/libip"
#pragma	lib	"libip.a"

enum 
{
	IPaddrlen=	16,
	IPv4addrlen=	4,
	IPv4off=	12,
	IPllen=		4,
	IPV4HDR_LEN=	20,

	/* vihl & vcf[0] values */
	IP_VER4= 	0x40,
	IP_VER6=	0x60,
};

/*
 *  for reading /net/ipifc
 */
typedef struct Ipifc Ipifc;
typedef struct Iplifc Iplifc;
typedef struct Ipv6rp Ipv6rp;

/* local address */
struct Iplifc
{
	Iplifc	*next;

	/* per address on the ip interface */
	uchar	ip[IPaddrlen];
	uchar	mask[IPaddrlen];
	uchar	net[IPaddrlen];		/* ip & mask */
	ulong	preflt;			/* preferred lifetime */
	ulong	validlt;		/* valid lifetime */
};

/* default values, one per stack */
struct Ipv6rp
{
	int	mflag;
	int	oflag;
	int 	maxraint;
	int	minraint;
	int	linkmtu;
	int	reachtime;
	int	rxmitra;
	int	ttl;
	int	routerlt;	
};

/* actual interface */
struct Ipifc
{
	Ipifc	*next;
	Iplifc	*lifc;

	/* per ip interface */
	int	index;			/* number of interface in ipifc dir */
	char	dev[64];
	uchar	sendra6;		/* on == send router adv */
	uchar	recvra6;		/* on == rcv router adv */
	int	mtu;
	ulong	pktin;
	ulong	pktout;
	ulong	errin;
	ulong	errout;
	Ipv6rp	rp;
};

#define ISIPV6MCAST(addr)	((addr)[0] == 0xff)
#define ISIPV6LINKLOCAL(addr) ((addr)[0] == 0xfe && ((addr)[1] & 0xc0) == 0x80)

/*
 * ipv6 constants
 * `ra' is `router advertisement', `rs' is `router solicitation'.
 * `na' is `neighbour advertisement'.
 */
enum {
	IPV6HDR_LEN	= 40,

	/* neighbour discovery option types */
	V6nd_srclladdr	= 1,
	V6nd_targlladdr	= 2,
	V6nd_pfxinfo	= 3,
	V6nd_redirhdr	= 4,
	V6nd_mtu	= 5,
	/* new since rfc2461; see iana.org/assignments/icmpv6-parameters */
	V6nd_home	= 8,
	V6nd_srcaddrs	= 9,		/* rfc3122 */
	V6nd_ip		= 17,
	/* /lib/rfc/drafts/draft-jeong-dnsop-ipv6-dns-discovery-12.txt */
	V6nd_rdns	= 25,
	/* plan 9 extensions */
	V6nd_9fs	= 250,
	V6nd_9auth	= 251,

	/* Router constants (all times in ms.) */
	Maxv6initraintvl= 16000,
	Maxv6initras	= 3,
	Maxv6finalras	= 3,
	Minv6interradelay= 3000,
	Maxv6radelay	= 500,

	/* Host constants */
	Maxv6rsdelay	= 1000,
	V6rsintvl	= 4000,
	Maxv6rss	= 3,

	/* Node constants */
	Maxv6mcastrss	= 3,
	Maxv6unicastrss	= 3,
	Maxv6anycastdelay= 1000,
	Maxv6na		= 3,
	V6reachabletime	= 30000,
	V6retranstimer	= 1000,
	V6initprobedelay= 5000,
};

/* V6 header on the wire */
typedef struct Ip6hdr Ip6hdr;
struct Ip6hdr {
	uchar	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	uchar	ploadlen[2];	/* payload length: packet length - 40 */
	uchar	proto;		/* next header type */
	uchar	ttl;		/* hop limit */
	uchar	src[IPaddrlen];	/* source address */
	uchar	dst[IPaddrlen];	/* destination address */
	uchar	payload[];
};

/*
 *  user-level icmpv6 with control message "headers"
 */
typedef struct Icmp6hdr Icmp6hdr;
struct Icmp6hdr {
	uchar	_0_[8];
	uchar	laddr[IPaddrlen];	/* local address */
	uchar	raddr[IPaddrlen];	/* remote address */
};

/*
 *  user level udp headers with control message "headers"
 */
enum 
{
	Udphdrsize=	52,	/* size of a Udphdr */
};

typedef struct Udphdr Udphdr;
struct Udphdr
{
	uchar	raddr[IPaddrlen];	/* V6 remote address */
	uchar	laddr[IPaddrlen];	/* V6 local address */
	uchar	ifcaddr[IPaddrlen];	/* V6 ifc addr msg was received on */
	uchar	rport[2];		/* remote port */
	uchar	lport[2];		/* local port */
};

uchar*	defmask(uchar*);
void	maskip(uchar*, uchar*, uchar*);
int	eipfmt(Fmt*);
int	isv4(uchar*);
vlong	parseip(uchar*, char*);
vlong	parseipmask(uchar*, char*);
char*	v4parseip(uchar*, char*);
char*	v4parsecidr(uchar*, uchar*, char*);
int	parseether(uchar*, char*);
int	myipaddr(uchar*, char*);
int	myetheraddr(uchar*, char*);
int	equivip4(uchar*, uchar*);
int	equivip6(uchar*, uchar*);

Ipifc*	readipifc(char*, Ipifc*, int);

void	hnputv(void*, uvlong);
void	hnputl(void*, uint);
void	hnputs(void*, ushort);
uvlong	nhgetv(void*);
uint	nhgetl(void*);
ushort	nhgets(void*);
ushort	ptclbsum(uchar*, int);

int	v6tov4(uchar*, uchar*);
void	v4tov6(uchar*, uchar*);

#define	ipcmp(x, y) memcmp(x, y, IPaddrlen)
#define	ipmove(x, y) memmove(x, y, IPaddrlen)

extern uchar IPv4bcast[IPaddrlen];
extern uchar IPv4bcastobs[IPaddrlen];
extern uchar IPv4allsys[IPaddrlen];
extern uchar IPv4allrouter[IPaddrlen];
extern uchar IPnoaddr[IPaddrlen];
extern uchar v4prefix[IPaddrlen];
extern uchar IPallbits[IPaddrlen];

#define CLASS(p) ((*(uchar*)(p))>>6)

#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"V"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"M"	uchar*
