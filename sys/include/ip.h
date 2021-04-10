/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


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
	u8	ip[IPaddrlen];
	u8	mask[IPaddrlen];
	u8	net[IPaddrlen];		/* ip & mask */
	u32	preflt;			/* preferred lifetime */
	u32	validlt;		/* valid lifetime */
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
	u8	sendra6;		/* on == send router adv */
	u8	recvra6;		/* on == rcv router adv */
	int	mtu;
	u32	pktin;
	u32	pktout;
	u32	errin;
	u32	errout;
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
	u8	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	u8	ploadlen[2];	/* payload length: packet length - 40 */
	u8	proto;		/* next header type */
	u8	ttl;		/* hop limit */
	u8	src[IPaddrlen];	/* source address */
	u8	dst[IPaddrlen];	/* destination address */
	u8	payload[];
};

/*
 *  user-level icmpv6 with control message "headers"
 */
typedef struct Icmp6hdr Icmp6hdr;
struct Icmp6hdr {
	u8	_0_[8];
	u8	laddr[IPaddrlen];	/* local address */
	u8	raddr[IPaddrlen];	/* remote address */
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
	u8	raddr[IPaddrlen];	/* V6 remote address */
	u8	laddr[IPaddrlen];	/* V6 local address */
	u8	ifcaddr[IPaddrlen];	/* V6 ifc addr msg was received on */
	u8	rport[2];		/* remote port */
	u8	lport[2];		/* local port */
};

u8 *	defmask(u8*);
void	maskip(u8*, u8*, u8*);
int	eipfmt(Fmt*);
int	isv4(u8*);
i64	parseip(u8*, char*);
i64	parseipmask(u8*, char*);
char*	v4parseip(u8*, char*);
char*	v4parsecidr(u8*, u8*, char*);
int	parseether(u8*, char*);
int	myipaddr(u8*, char*);
int	myetheraddr(u8*, char*);
int	equivip4(u8*, u8*);
int	equivip6(u8*, u8*);

Ipifc*	readipifc(char*, Ipifc*, int);

void	hnputv(void*, u64);
void	hnputl(void*, uint);
void	hnputs(void*, u16);
u64	nhgetv(void*);
uint	nhgetl(void*);
u16	nhgets(void*);
u16	ptclbsum(u8*, int);

int	v6tov4(u8*, u8*);
void	v4tov6(u8*, u8*);

#define	ipcmp(x, y) memcmp(x, y, IPaddrlen)
#define	ipmove(x, y) memmove(x, y, IPaddrlen)

extern u8 IPv4bcast[IPaddrlen];
extern u8 IPv4bcastobs[IPaddrlen];
extern u8 IPv4allsys[IPaddrlen];
extern u8 IPv4allrouter[IPaddrlen];
extern u8 IPnoaddr[IPaddrlen];
extern u8 v4prefix[IPaddrlen];
extern u8 IPallbits[IPaddrlen];

#define CLASS(p) ((*(u8*)(p))>>6)

