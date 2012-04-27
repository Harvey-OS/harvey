/*
 * Internet Protocol Version 6
 *
 * rfc2460 defines the protocol, rfc2461 neighbour discovery, and
 * rfc2462 address autoconfiguration.  rfc4443 defines ICMP; was rfc2463.
 * rfc4291 defines the address architecture (including prefices), was rfc3513.
 * rfc4007 defines the scoped address architecture.
 *
 * global unicast is anything but unspecified (::), loopback (::1),
 * multicast (ff00::/8), and link-local unicast (fe80::/10).
 *
 * site-local (fec0::/10) is now deprecated, originally by rfc3879.
 *
 * Unique Local IPv6 Unicast Addresses are defined by rfc4193.
 * prefix is fc00::/7, scope is global, routing is limited to roughly a site.
 */
#define isv6mcast(addr)	  ((addr)[0] == 0xff)
#define islinklocal(addr) ((addr)[0] == 0xfe && ((addr)[1] & 0xc0) == 0x80)

#define optexsts(np)	(nhgets((np)->ploadlen) > 24)
#define issmcast(addr)	(memcmp((addr), v6solicitednode, 13) == 0)

#ifndef MIN
#define MIN(a, b) ((a) <= (b)? (a): (b))
#endif

enum {				/* Header Types */
	HBH		= 0,	/* hop-by-hop multicast routing protocol */
	ICMP		= 1,
	IGMP		= 2,
	GGP		= 3,
	IPINIP		= 4,
	ST		= 5,
	TCP		= 6,
	UDP		= 17,
	ISO_TP4		= 29,
	RH		= 43,
	FH		= 44,
	IDRP		= 45,
	RSVP		= 46,
	AH		= 51,
	ESP		= 52,
	ICMPv6		= 58,
	NNH		= 59,
	DOH		= 60,
	ISO_IP		= 80,
	IGRP		= 88,
	OSPF		= 89,

	Maxhdrtype	= 256,
};

enum {
	/* multicast flags and scopes */

//	Well_known_flg	= 0,
//	Transient_flg	= 1,

//	Interface_local_scop = 1,
	Link_local_scop	= 2,
//	Site_local_scop	= 5,
//	Org_local_scop	= 8,
	Global_scop	= 14,

	/* various prefix lengths */
	SOLN_PREF_LEN	= 13,

	/* icmpv6 unreachability codes */
	Icmp6_no_route		= 0,
	Icmp6_ad_prohib		= 1,
	Icmp6_out_src_scope	= 2,
	Icmp6_adr_unreach	= 3,
	Icmp6_port_unreach	= 4,
	Icmp6_gress_src_fail	= 5,
	Icmp6_rej_route		= 6,
	Icmp6_unknown		= 7,  /* our own invention for internal use */

	/* various flags & constants */
	v6MINTU		= 1280,
	HOP_LIMIT	= 255,
	IP6HDR		= 40,		/* sizeof(Ip6hdr) = 8 + 2*16 */

	/* option types */

	/* neighbour discovery */
	SRC_LLADDR	= 1,
	TARGET_LLADDR	= 2,
	PREFIX_INFO	= 3,
	REDIR_HEADER	= 4,
	MTU_OPTION	= 5,
	/* new since rfc2461; see iana.org/assignments/icmpv6-parameters */
	V6nd_home	= 8,
	V6nd_srcaddrs	= 9,		/* rfc3122 */
	V6nd_ip		= 17,
	/* /lib/rfc/drafts/draft-jeong-dnsop-ipv6-dns-discovery-12.txt */
	V6nd_rdns	= 25,
	/* plan 9 extensions */
	V6nd_9fs	= 250,
	V6nd_9auth	= 251,

	SRC_UNSPEC	= 0,
	SRC_UNI		= 1,
	TARG_UNI	= 2,
	TARG_MULTI	= 3,

	Tunitent	= 1,
	Tuniproxy	= 2,
	Tunirany	= 3,

	/* Node constants */
	MAX_MULTICAST_SOLICIT	= 3,
	RETRANS_TIMER		= 1000,
};

typedef struct Ip6hdr	Ip6hdr;
typedef struct Opthdr	Opthdr;
typedef struct Routinghdr Routinghdr;
typedef struct Fraghdr6	Fraghdr6;

/* we do this in case there's padding at the end of Ip6hdr */
#define IPV6HDR \
	uchar	vcf[4];		/* version:4, traffic class:8, flow label:20 */\
	uchar	ploadlen[2];	/* payload length: packet length - 40 */ \
	uchar	proto;		/* next header type */ \
	uchar	ttl;		/* hop limit */ \
	uchar	src[IPaddrlen]; \
	uchar	dst[IPaddrlen]

struct	Ip6hdr {
	IPV6HDR;
	uchar	payload[];
};

struct	Opthdr {		/* unused */
	uchar	nexthdr;
	uchar	len;
};

/*
 * Beware routing header type 0 (loose source routing); see
 * http://www.secdev.org/conf/IPv6_RH_security-csw07.pdf.
 * Type 1 is unused.  Type 2 is for MIPv6 (mobile IPv6) filtering
 * against type 0 header.
 */
struct	Routinghdr {		/* unused */
	uchar	nexthdr;
	uchar	len;
	uchar	rtetype;
	uchar	segrem;
};

struct	Fraghdr6 {
	uchar	nexthdr;
	uchar	res;
	uchar	offsetRM[2];	/* Offset, Res, M flag */
	uchar	id[4];
};

extern uchar v6allnodesN[IPaddrlen];
extern uchar v6allnodesL[IPaddrlen];
extern uchar v6allroutersN[IPaddrlen];
extern uchar v6allroutersL[IPaddrlen];
extern uchar v6allnodesNmask[IPaddrlen];
extern uchar v6allnodesLmask[IPaddrlen];
extern uchar v6solicitednode[IPaddrlen];
extern uchar v6solicitednodemask[IPaddrlen];
extern uchar v6Unspecified[IPaddrlen];
extern uchar v6loopback[IPaddrlen];
extern uchar v6loopbackmask[IPaddrlen];
extern uchar v6linklocal[IPaddrlen];
extern uchar v6linklocalmask[IPaddrlen];
extern uchar v6multicast[IPaddrlen];
extern uchar v6multicastmask[IPaddrlen];

extern int v6llpreflen;
extern int v6mcpreflen;
extern int v6snpreflen;
extern int v6aNpreflen;
extern int v6aLpreflen;

extern int ReTransTimer;

void ipv62smcast(uchar *, uchar *);
void icmpns(Fs *f, uchar* src, int suni, uchar* targ, int tuni, uchar* mac);
void icmpna(Fs *f, uchar* src, uchar* dst, uchar* targ, uchar* mac, uchar flags);
void icmpttlexceeded6(Fs *f, Ipifc *ifc, Block *bp);
void icmppkttoobig6(Fs *f, Ipifc *ifc, Block *bp);
void icmphostunr(Fs *f, Ipifc *ifc, Block *bp, int code, int free);
