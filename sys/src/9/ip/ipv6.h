#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/* rfc 3513 defines the address prefices */
#define isv6mcast(addr)	  ((addr)[0] == 0xff)
#define islinklocal(addr) ((addr)[0] == 0xfe && ((addr)[1] & 0xc0) == 0x80)
#define issitelocal(addr) ((addr)[0] == 0xfe && ((addr)[1] & 0xc0) == 0xc0)
#define isv6global(addr) (((addr)[0] & 0xe0) == 0x20)

#define optexsts(np) (nhgets((np)->ploadlen) > 24)
#define issmcast(addr) (memcmp((addr), v6solicitednode, 13) == 0)

/* from RFC 2460 */

typedef struct Ip6hdr     Ip6hdr;
typedef struct Opthdr     Opthdr;
typedef struct Routinghdr Routinghdr;
typedef struct Fraghdr6    Fraghdr6;

struct Ip6hdr {
	uchar vcf[4];       	// version:4, traffic class:8, flow label:20
	uchar ploadlen[2];  	// payload length: packet length - 40
	uchar proto;		// next header type
	uchar ttl;          	// hop limit
	uchar src[IPaddrlen];
	uchar dst[IPaddrlen];
};

struct Opthdr {
	uchar nexthdr;
	uchar len;
};

struct Routinghdr {
	uchar nexthdr;
	uchar len;
	uchar rtetype;
	uchar segrem;
};

struct Fraghdr6 {
	uchar nexthdr;
	uchar res;
	uchar offsetRM[2];	// Offset, Res, M flag
	uchar id[4];
};


enum {			/* Header Types */
	HBH		= 0,	//?
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
	//	multicast flgs and scop

	well_known_flg				= 0,
	transient_flg				= 1,

	node_local_scop 			= 1,
	link_local_scop 			= 2,
	site_local_scop 			= 5,
	org_local_scop				= 8,
	global_scop				= 14,

	//	various prefix lengths

	SOLN_PREF_LEN				= 13,

	//	icmpv6 unreach codes
	icmp6_no_route				= 0,
	icmp6_ad_prohib				= 1,
	icmp6_unassigned			= 2,
	icmp6_adr_unreach			= 3,
	icmp6_port_unreach			= 4,
	icmp6_unkn_code				= 5,

	// 	various flags & constants

	v6MINTU      				= 1280,	
	HOP_LIMIT    				= 255,
	ETHERHDR_LEN 				= 14,
	IPV6HDR_LEN  				= 40,
	IPV4HDR_LEN  				= 20,

	// 	option types

	SRC_LLADDRESS    			= 1,
	TARGET_LLADDRESS 			= 2,
	PREFIX_INFO      			= 3,
	REDIR_HEADER     			= 4,
	MTU_OPTION       			= 5,

	SRC_UNSPEC  				= 0,
	SRC_UNI     				= 1, 
	TARG_UNI    				= 2,
	TARG_MULTI  				= 3,

	t_unitent   				= 1,
	t_uniproxy  				= 2,
	t_unirany   				= 3,

	//	Router constants (all times in milliseconds)

	MAX_INITIAL_RTR_ADVERT_INTERVAL 	= 16000,
	MAX_INITIAL_RTR_ADVERTISEMENTS  	= 3, 
	MAX_FINAL_RTR_ADVERTISEMENTS    	= 3,
	MIN_DELAY_BETWEEN_RAS 			= 3000,
	MAX_RA_DELAY_TIME     			= 500,

	//	Host constants

	MAX_RTR_SOLICITATION_DELAY 		= 1000,
	RTR_SOLICITATION_INTERVAL  		= 4000,
	MAX_RTR_SOLICITATIONS      		= 3,

	//	Node constants

	MAX_MULTICAST_SOLICIT   		= 3,
	MAX_UNICAST_SOLICIT     		= 3,
	MAX_ANYCAST_DELAY_TIME  		= 1000,
	MAX_NEIGHBOR_ADVERTISEMENT 		= 3,	
	REACHABLE_TIME 				= 30000,
	RETRANS_TIMER  				= 1000,	
	DELAY_FIRST_PROBE_TIME 			= 5000,

};

extern void ipv62smcast(uchar *, uchar *);
extern void icmpns(Fs *f, uchar* src, int suni, uchar* targ, int tuni, uchar* mac);
extern void icmpna(Fs *f, uchar* src, uchar* dst, uchar* targ, uchar* mac, uchar flags);
extern void icmpttlexceeded6(Fs *f, Ipifc *ifc, Block *bp);
extern void icmppkttoobig6(Fs *f, Ipifc *ifc, Block *bp);
extern void icmphostunr(Fs *f, Ipifc *ifc, Block *bp, int code, int free);

extern uchar v6allnodesN[IPaddrlen];
extern uchar v6allnodesL[IPaddrlen];
extern uchar v6allroutersN[IPaddrlen];
extern uchar v6allroutersL[IPaddrlen];
extern uchar v6allnodesNmask[IPaddrlen];
extern uchar v6allnodesLmask[IPaddrlen];
extern uchar v6allroutersS[IPaddrlen];
extern uchar v6solicitednode[IPaddrlen];
extern uchar v6solicitednodemask[IPaddrlen];
extern uchar v6Unspecified[IPaddrlen];
extern uchar v6loopback[IPaddrlen];
extern uchar v6loopbackmask[IPaddrlen];
extern uchar v6linklocal[IPaddrlen];
extern uchar v6linklocalmask[IPaddrlen];
extern uchar v6sitelocal[IPaddrlen];
extern uchar v6sitelocalmask[IPaddrlen];
extern uchar v6glunicast[IPaddrlen];
extern uchar v6multicast[IPaddrlen];
extern uchar v6multicastmask[IPaddrlen];

extern int v6llpreflen;
extern int v6slpreflen;
extern int v6lbpreflen;
extern int v6mcpreflen;
extern int v6snpreflen;
extern int v6aNpreflen;
extern int v6aLpreflen;

extern int ReTransTimer;
