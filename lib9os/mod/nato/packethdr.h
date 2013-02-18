// packet headers

enum{
	// ETHER type
	ETHER_0132	= 0x0132, // unknown, but observed on our link
	ETHER_6002	= 0x6002, // DEC remote console?
	ETHER_IPv4	= 0x0800,
	ETHER_IPv6	= 0x86dd,
	ETHER_ARP	= 0x0806,
	ETHER_RARP	= 0x8035,
	ETHER_SNMP	= 0x814c,
	ETHER_LOOPBACK	= 0x9000,

	// ARP op
	ARP_request	= 1,
	ARP_reply	= 2,
	RARP_request	= 3,
	RARP_reply	= 4,

//	IP_VER		= 0x40,		// IP version 4
//	IP_VER6		= 0x60,		// IP version 4
	IP_HLEN		= 0x05,		// header length in words
	IP_DF		= 0x4000,	// don't fragment
	IP_MF		= 0x2000,	// more fragments
	IP_MAX		= (32*1024),	// max Internet packet size

	// IP proto
	IP_HOPBYHOP	= 0,
	IP_ICMPPROTO	= 1,
	IP_IGMPPROTO	= 2,
	IP_TCPPROTO	= 6,
	IP_UDPPROTO	= 17,
	IP_ILPROTO	= 40,
	IP_v6ROUTE	= 43,
	IP_v6FRAG	= 44,
	IP_IPsecESP	= 50,
	IP_IPsecAH	= 51,
	IP_v6ICMP	= 58,
	IP_v6NOMORE	= 59,

	// TCP flag
	TCP_URG		= 0x20,
	TCP_ACK		= 0x10,
	TCP_PSH		= 0x08,
	TCP_RST		= 0x04,
	TCP_SYN		= 0x02,
	TCP_FIN		= 0x01,

	// ICMP type
	ICMP_echoreply	= 0,
	ICMP_unreach	= 3,
	ICMP_srcquench	= 4,
	ICMP_redirect	= 5,
	ICMP_echoreq	= 8,
	ICMP_ttl	= 11,
	ICMP_bad	= 12,
	ICMP_timereq	= 13,
	ICMP_timereply	= 14,
	ICMP_maskreq	= 17,
	ICMP_maskreply	= 18,
};

typedef struct ETHER{
	uchar dstmac[6];
	uchar srcmac[6];
	uchar type[2];
} ETHER;

typedef struct ARP{
	uchar hdr[6];	// for IP, 0x 0001 8000 0604
	uchar op[2];
	uchar srcmac[6];
	uchar srcaddr[4];
	uchar dstmac[6];
	uchar dstaddr[4];
} ARP;

typedef uchar Ipaddr[IPaddrlen];
// Most of the nat code is specific to IPv4, so Ipaddr is not used much yet.

typedef struct IP6 {
	uchar	vcf[4];		// ver:4, class:8, flow:20
	uchar	length[2];	// packet length - 40
	uchar	proto;		// type of next header
	uchar	ttl;		// "hop limit"
	uchar	src[IPaddrlen];
	uchar	dst[IPaddrlen];
	// extension headers
	//	Hop-by-Hop Options (unspecified TLV list)
	//	Destination Options (first and route dst)
	//	Routing
	//	Fragment
	//	Authentication
	//	Encapsulating Security Payload
	//	Destination Options (final)
	//	upper-layer header (transport or tunneled IP)
} IP6;

typedef struct IP {
	uchar	vihl;		// Version and header length
	uchar	tos;		// Type of service
	uchar	length[2];	// packet length
	uchar	id[2];		// ip->identification
	uchar	frag[2];	// Fragment information
	uchar	ttl;		// Time to live
	uchar	proto;		// Protocol
	uchar	cksum[2];	// Header checksum
	uchar	src[4];		// IP source
	uchar	dst[4];		// IP destination
} IP;

typedef struct PseudoHdr{
	uchar	src[4];
	uchar	dst[4];
	uchar	zero;
	uchar	proto;
	uchar	length[2];
	uchar	hdrdata[1580];
} PseudoHdr;

typedef struct UDP{
	uchar	udpsport[2];	// Source port
	uchar	udpdport[2];	// Destination port
	uchar	udplen[2];	// data length
	uchar	udpcksum[2];	// Checksum
} UDP;

typedef struct TCP{
	uchar	tcpsport[2];
	uchar	tcpdport[2];
	uchar	tcpseq[4];
	uchar	tcpack[4];
	uchar	tcpflag[2];	// 4bit hdrlen, 6bit 0, 6bit flags
	uchar	tcpwin[2];
	uchar	tcpcksum[2];
	uchar	tcpurg[2];
	// Options segment
	uchar	tcpopt[2];
	uchar	tcpmss[2];
} TCP;

typedef struct IL{
	uchar	ilsum[2];	// Checksum including header
	uchar	illen[2];	// Packet length
	uchar	iltype;		// Packet type
	uchar	ilspec;		// Special
	uchar	ilsrc[2];	// Src port
	uchar	ildst[2];	// Dst port
	uchar	ilid[4];	// Sequence id
	uchar	ilack[4];	// Acked sequence
} IL;

typedef struct ICMP{
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
} ICMP;

