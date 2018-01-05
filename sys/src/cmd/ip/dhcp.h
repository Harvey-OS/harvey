/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Dynamic Host Configuration Protocol / BOOTP */
enum
{
	OfferTimeout=	60,		/* when an offer times out */
	MaxLease=	60*60,		/* longest lease for dynamic binding */
	MinLease=	15*60,		/* shortest lease for dynamic binding */
	StaticLease=	30*60,		/* lease for static binding */

	IPUDPHDRSIZE=	28,		/* size of an IP plus UDP header */
	MINSUPPORTED=	576,		/* biggest IP message the client must support */

	/* lengths of some bootp fields */
	Maxhwlen=	16,
	Maxfilelen=	128,
	Maxoptlen=	312-4,

	/* bootp types */
	Bootrequest=	1,
	Bootreply= 	2,

	/* bootp flags */
	Fbroadcast=	1<<15,

	/* dhcp v4 types */
	Discover=	1,
	Offer=		2,
	Request=	3,
	Decline=	4,
	Ack=		5,
	Nak=		6,
	Release=	7,
	Inform=		8,

	/* bootp option types */
	OBend=			255,
	OBpad=			0,
	OBmask=			1,
	OBtimeoff=		2,
	OBrouter=		3,
	OBtimeserver=		4,
	OBnameserver=		5,
	OBdnserver=		6,
	OBlogserver=		7,
	OBcookieserver=		8,
	OBlprserver=		9,
	OBimpressserver=	10,
	OBrlserver=		11,
	OBhostname=		12,	/* 0x0c */
	OBbflen=		13,
	OBdumpfile=		14,
	OBdomainname=		15,
	OBswapserver=		16,	/* 0x10 */
	OBrootpath=		17,
	OBextpath=		18,
	OBipforward=		19,
	OBnonlocal=		20,
	OBpolicyfilter=		21,
	OBmaxdatagram=		22,
	OBttl=			23,
	OBpathtimeout=		24,
	OBpathplateau=		25,
	OBmtu=			26,
	OBsubnetslocal=		27,
	OBbaddr=		28,
	OBdiscovermask=		29,
	OBsupplymask=		30,
	OBdiscoverrouter=	31,
	OBrsserver=		32,	/* 0x20 */
	OBstaticroutes=		33,
	OBtrailerencap=		34,
	OBarptimeout=		35,
	OBetherencap=		36,
	OBtcpttl=		37,
	OBtcpka=		38,
	OBtcpkag=		39,
	OBnisdomain=		40,
	OBniserver=		41,
	OBntpserver=		42,
	OBvendorinfo=		43,	/* 0x2b */
	OBnetbiosns=		44,
	OBnetbiosdds=		45,
	OBnetbiostype=		46,
	OBnetbiosscope=		47,
	OBxfontserver=		48,	/* 0x30 */
	OBxdispmanager=		49,
	OBnisplusdomain=	64,	/* 0x40 */
	OBnisplusserver=	65,
	OBhomeagent=		68,
	OBsmtpserver=		69,
	OBpop3server=		70,
	OBnntpserver=		71,
	OBwwwserver=		72,
	OBfingerserver=		73,
	OBircserver=		74,
	OBstserver=		75,
	OBstdaserver=		76,

	/* dhcp v4 options */
	ODipaddr=		50,	/* 0x32 */
	ODlease=		51,
	ODoverload=		52,
	ODtype=			53,	/* 0x35 */
	ODserverid=		54,	/* 0x36 */
	ODparams=		55,	/* 0x37 */
	ODmessage=		56,
	ODmaxmsg=		57,
	ODrenewaltime=		58,
	ODrebindingtime=	59,
	ODvendorclass=		60,
	ODclientid=		61,	/* 0x3d */
	ODtftpserver=		66,
	ODbootfile=		67,

	ODpxearch=		93,	/* see rfc 4578 */
	ODpxeni=		94,
	ODpxeguid=		97,

	/* plan9 vendor info options, v4 addresses only (deprecated) */
	OP9fsv4=		128,	/* plan9 file servers */
	OP9authv4=		129,	/* plan9 auth servers */

	/* plan9 vendor info options, textual addresses, thus v4 or v6 */
	OP9fs=			130,	/* plan9 file servers */
	OP9auth=		131,	/* plan9 auth servers */
	OP9ipaddr=		132,	/* client's address */
	OP9ipmask=		133,	/* client's subnet mask */
	OP9ipgw=		134,	/* client's gateway */
/*	OP9dns=			135,*/	/* dns servers */
};

/* a lease that never expires. DHCP specifies that it is 32 bits of seconds. */
#define Lforever	((uint32_t)~0)

/* dhcp states */
enum {
	Sinit,
	Sselecting,
	Srequesting,
	Sbound,
	Srenewing,
	Srebinding,
};

typedef struct Bootp	Bootp;
struct Bootp
{
	/* Udphdr (included because of structure alignment on the alpha) */
	unsigned char	udphdr[Udphdrsize];

	unsigned char	op;			/* opcode */
	unsigned char	htype;			/* hardware type */
	unsigned char	hlen;			/* hardware address len */
	unsigned char	hops;			/* hops */
	unsigned char	xid[4];			/* a random number */
	unsigned char	secs[2];		/* elapsed since client started booting */
	unsigned char	flags[2];
	unsigned char	ciaddr[IPv4addrlen];	/* client IP address (client tells server) */
	unsigned char	yiaddr[IPv4addrlen];	/* client IP address (server tells client) */
	unsigned char	siaddr[IPv4addrlen];	/* server IP address */
	unsigned char	giaddr[IPv4addrlen];	/* gateway IP address */
	unsigned char	chaddr[Maxhwlen];	/* client hardware address */
	char	sname[64];		/* server host name (optional) */
	char	file[Maxfilelen];	/* boot file name */
	unsigned char	optmagic[4];
	unsigned char	optdata[Maxoptlen];
};
