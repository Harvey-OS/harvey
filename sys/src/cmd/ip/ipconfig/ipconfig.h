/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Conf Conf;
typedef struct Ctl Ctl;

struct Conf
{
	/* locally generated */
	char	*type;
	char	*dev;
	char	mpoint[32];
	int	cfd;			/* ifc control channel */
	int	dfd;			/* ifc data channel (for ppp) */
	char	*cputype;
	unsigned char	hwa[32];		/* hardware address */
	int	hwatype;
	int	hwalen;
	unsigned char	cid[32];
	int	cidlen;
	char	*baud;

	/* learned info */
	unsigned char	gaddr[IPaddrlen];
	unsigned char	laddr[IPaddrlen];
	unsigned char	mask[IPaddrlen];
	unsigned char	raddr[IPaddrlen];
	unsigned char	dns[2*IPaddrlen];
	unsigned char	fs[2*IPaddrlen];
	unsigned char	auth[2*IPaddrlen];
	unsigned char	ntp[IPaddrlen];
	int	mtu;

	/* dhcp specific */
	int	state;
	int	fd;
	uint32_t	xid;
	uint32_t	starttime;
	char	sname[64];
	char	hostname[32];
	char	domainname[64];
	unsigned char	server[IPaddrlen];	/* server IP address */
	uint32_t	offered;		/* offered lease time */
	uint32_t	lease;			/* lease time */
	uint32_t	resend;			/* # of resends for current state */
	uint32_t	timeout;		/* time to timeout - seconds */

	/*
	 * IPv6
	 */

	/* solicitation specific - XXX add support for IPv6 leases */
//	ulong	solicit_retries;

	/* router-advertisement related */
	unsigned char	sendra;
	unsigned char	recvra;
	unsigned char	mflag;
	unsigned char	oflag;
	int 	maxraint; /* rfc2461, p.39: 4sec ≤ maxraint ≤ 1800sec, def 600 */
	int	minraint;	/* 3sec ≤ minraint ≤ 0.75*maxraint */
	int	linkmtu;
	int	reachtime;	/* 3,600,000 msec, default 0 */
	int	rxmitra;	/* default 0 */
	int	ttl;		/* default 0 (unspecified) */
	/* default gateway params */
	unsigned char	v6gaddr[IPaddrlen];
	int	routerlt;	/* router life time */

	/* prefix related */
	unsigned char	v6pref[IPaddrlen];
	int	prefixlen;
	unsigned char	onlink;		/* flag: address is `on-link' */
	unsigned char	autoflag;	/* flag: autonomous */
	uint32_t	validlt;	/* valid lifetime (seconds) */
	uint32_t	preflt;		/* preferred lifetime (seconds) */
};

struct Ctl
{
	Ctl	*next;
	char	*ctl;
};

extern Ctl *firstctl, **ctll;

extern Conf conf;

extern int	noconfig;
extern int	ipv6auto;
extern int	debug;
extern int	dodhcp;
extern int	dolog;
extern int	nip;
extern int	plan9;
extern int	dupl_disc;

extern int	myifc;
extern char	*vs;

void	adddefroute(char*, unsigned char*);
void	binddevice(void);
void	bootprequest(void);
void	controldevice(void);
void	dhcpquery(int, int);
void	dhcprecv(void);
void	dhcpsend(int);
int	dhcptimer(void);
void	dhcpwatch(int);
void	doadd(int);
void	doremove(void);
void	dounbind(void);
int	getndb(void);
int	ipconfig4(void);
int	ipconfig6(int);
int32_t	jitter(void);
void	lookforip(char*);
void	mkclientid(void);
int	nipifcs(char*);
int	openlisten(void);
unsigned char	*optaddaddr(unsigned char*, int, unsigned char*);
unsigned char	*optaddbyte(unsigned char*, int, int);
unsigned char	*optaddstr(unsigned char*, int, char*);
unsigned char	*optadd(unsigned char*, int, void*, int);
unsigned char	*optaddulong(unsigned char*, int, uint32_t);
unsigned char	*optaddvec(unsigned char*, int, unsigned char*, int);
int	optgetaddrs(unsigned char*, int, unsigned char*, int);
int	optgetaddr(unsigned char*, int, unsigned char*);
int	optgetbyte(unsigned char*, int);
int	optgetstr(unsigned char*, int, char*, int);
unsigned char	*optget(unsigned char*, int, int*);
uint32_t	optgetulong(unsigned char*, int);
int	optgetvec(unsigned char*, int, unsigned char*, int);
int	parseoptions(unsigned char *p, int n);
int	parseverb(char*);
void	procsetname(char *fmt, ...);
void	putndb(void);
uint32_t	randint(uint32_t low, uint32_t hi);
void	tweakservers(void);
void	usage(void);
int	validip(unsigned char*);
void	warning(char *fmt, ...);
void	writendb(char*, int, int);

/*
 * IPv6
 */

void	doipv6(int);
int	ipconfig6(int);
void	recvra6(void);
void	sendra6(void);
void	v6paraminit(Conf *);

typedef struct Headers Headers;
typedef struct Ip4hdr  Ip4hdr;
typedef struct Lladdropt Lladdropt;
typedef struct Mtuopt Mtuopt;
typedef struct Prefixopt Prefixopt;
typedef struct Routeradv Routeradv;
typedef struct Routersol Routersol;

enum {
	IsRouter 	= 1,
	IsHostRecv	= 2,
	IsHostNoRecv	= 3,

	MAClen		= 6,

	IPv4		= 4,
	IPv6		= 6,
	Defmtu		= 1400,

	IP_HOPBYHOP	= 0,
	ICMPv4		= 1,
	IP_IGMPPROTO	= 2,
	IP_TCPPROTO	= 6,
	IP_UDPPROTO	= 17,
	IP_ILPROTO	= 40,
	IP_v6ROUTE	= 43,
	IP_v6FRAG	= 44,
	IP_IPsecESP	= 50,
	IP_IPsecAH	= 51,
	IP_v6NOMORE	= 59,
	ICMP6_RS	= 133,
	ICMP6_RA	= 134,

	IP_IN_IP	= 41,
};

enum {
	MFMASK = 1 << 7,
	OCMASK = 1 << 6,
	OLMASK = 1 << 7,
	AFMASK = 1 << 6,
};

enum {
	MAXTTL		= 255,
	D64HLEN		= IPV6HDR_LEN - IPV4HDR_LEN,
	IP_MAX		= 32*1024,
};

struct Headers {
	unsigned char	dst[IPaddrlen];
	unsigned char	src[IPaddrlen];
};

struct Routersol {
	unsigned char	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	unsigned char	ploadlen[2];	/* payload length: packet length - 40 */
	unsigned char	proto;		/* next header	type */
	unsigned char	ttl;		/* hop limit */
	unsigned char	src[IPaddrlen];
	unsigned char	dst[IPaddrlen];
	unsigned char	type;
	unsigned char	code;
	unsigned char	cksum[2];
	unsigned char	res[4];
};

struct Routeradv {
	unsigned char	vcf[4];		/* version:4, traffic class:8, flow label:20 */
	unsigned char	ploadlen[2];	/* payload length: packet length - 40 */
	unsigned char	proto;		/* next header	type */
	unsigned char	ttl;		/* hop limit */
	unsigned char	src[IPaddrlen];
	unsigned char	dst[IPaddrlen];
	unsigned char	type;
	unsigned char	code;
	unsigned char	cksum[2];
	unsigned char	cttl;
	unsigned char	mor;
	unsigned char	routerlt[2];
	unsigned char	rchbltime[4];
	unsigned char	rxmtimer[4];
};

struct Lladdropt {
	unsigned char	type;
	unsigned char	len;
	unsigned char	lladdr[MAClen];
};

struct Prefixopt {
	unsigned char	type;
	unsigned char	len;
	unsigned char	plen;
	unsigned char	lar;
	unsigned char	validlt[4];
	unsigned char	preflt[4];
	unsigned char	reserv[4];
	unsigned char	pref[IPaddrlen];
};

struct Mtuopt {
	unsigned char	type;
	unsigned char	len;
	unsigned char	reserv[2];
	unsigned char	mtu[4];
};

void	ea2lla(unsigned char *lla, unsigned char *ea);
void	ipv62smcast(unsigned char *smcast, unsigned char *a);
