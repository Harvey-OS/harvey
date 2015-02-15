/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	uint8_t	ip[IPaddrlen];
	uint8_t	mask[IPaddrlen];
	uint8_t	net[IPaddrlen];		/* ip & mask */
	uint32_t	preflt;			/* preferred lifetime */
	uint32_t	validlt;		/* valid lifetime */
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
	uint8_t	sendra6;		/* on == send router adv */
	uint8_t	recvra6;		/* on == rcv router adv */
	int	mtu;
	uint32_t	pktin;
	uint32_t	pktout;
	uint32_t	errin;
	uint32_t	errout;
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
	uint8_t	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	uint8_t	ploadlen[2];	/* payload length: packet length - 40 */
	uint8_t	proto;		/* next header type */
	uint8_t	ttl;		/* hop limit */
	uint8_t	src[IPaddrlen];	/* source address */
	uint8_t	dst[IPaddrlen];	/* destination address */
	uint8_t	payload[];
};

/*
 *  user-level icmpv6 with control message "headers"
 */
typedef struct Icmp6hdr Icmp6hdr;
struct Icmp6hdr {
	uint8_t	_0_[8];
	uint8_t	laddr[IPaddrlen];	/* local address */
	uint8_t	raddr[IPaddrlen];	/* remote address */
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
	uint8_t	raddr[IPaddrlen];	/* V6 remote address */
	uint8_t	laddr[IPaddrlen];	/* V6 local address */
	uint8_t	ifcaddr[IPaddrlen];	/* V6 ifc addr msg was received on */
	uint8_t	rport[2];		/* remote port */
	uint8_t	lport[2];		/* local port */
};

uint8_t*	defmask(uint8_t*);
void	maskip(uint8_t*, uint8_t*, uint8_t*);
int	eipfmt(Fmt*);
int	isv4(uint8_t*);
int64_t	parseip(uint8_t*, char*);
int64_t	parseipmask(uint8_t*, char*);
char*	v4parseip(uint8_t*, char*);
char*	v4parsecidr(uint8_t*, uint8_t*, char*);
int	parseether(uint8_t*, char*);
int	myipaddr(uint8_t*, char*);
int	myetheraddr(uint8_t*, char*);
int	equivip4(uint8_t*, uint8_t*);
int	equivip6(uint8_t*, uint8_t*);

Ipifc*	readipifc(char*, Ipifc*, int);

void	hnputv(void*, uint64_t);
void	hnputl(void*, uint);
void	hnputs(void*, uint16_t);
uint64_t	nhgetv(void*);
uint	nhgetl(void*);
uint16_t	nhgets(void*);
uint16_t	ptclbsum(uint8_t*, int);

int	v6tov4(uint8_t*, uint8_t*);
void	v4tov6(uint8_t*, uint8_t*);

#define	ipcmp(x, y) memcmp(x, y, IPaddrlen)
#define	ipmove(x, y) memmove(x, y, IPaddrlen)

extern uint8_t IPv4bcast[IPaddrlen];
extern uint8_t IPv4bcastobs[IPaddrlen];
extern uint8_t IPv4allsys[IPaddrlen];
extern uint8_t IPv4allrouter[IPaddrlen];
extern uint8_t IPnoaddr[IPaddrlen];
extern uint8_t v4prefix[IPaddrlen];
extern uint8_t IPallbits[IPaddrlen];

#define CLASS(p) ((*(uint8_t*)(p))>>6)

#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"V"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"M"	uchar*
