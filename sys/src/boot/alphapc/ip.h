typedef struct Udphdr Udphdr;
struct Udphdr
{
	uchar	d[6];		/* Ethernet destination */
	uchar	s[6];		/* Ethernet source */
	uchar	type[2];	/* Ethernet packet type */

	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */

	/* Udp pseudo ip really starts here */
	uchar	ttl;	
	uchar	udpproto;	/* Protocol */
	uchar	udpplen[2];	/* Header plus data length */
	uchar	udpsrc[4];	/* Ip source */
	uchar	udpdst[4];	/* Ip destination */
	uchar	udpsport[2];	/* Source port */
	uchar	udpdport[2];	/* Destination port */
	uchar	udplen[2];	/* data length */
	uchar	udpcksum[2];	/* Checksum */
};

typedef struct Etherhdr Etherhdr;
struct Etherhdr
{
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];

	/* Now we have the ip fields */
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
};

enum
{
	IP_VER		= 0x40,
	IP_HLEN		= 0x05,			
 	UDP_EHSIZE	= 22,
	UDP_PHDRSIZE	= 12,
	UDP_HDRSIZE	= 20,
	ETHER_HDR	= 14,
	IP_UDPPROTO	= 17,
	ET_IP		= 0x800,
	Bcastip		= 0xffffffff,
	BPportsrc	= 68,
	BPportdst	= 67,
	TFTPport	= 69,
	Timeout		= 5000,	/* milliseconds */
	Bootrequest 	= 1,
	Bootreply   	= 2,
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Segsize		= 512,
	TFTPSZ		= Segsize+10,
};

typedef struct Bootp Bootp;
struct Bootp
{
	uchar	op;		/* opcode */
	uchar	htype;		/* hardware type */
	uchar	hlen;		/* hardware address len */
	uchar	hops;		/* hops */
	uchar	xid[4];		/* a random number */
	uchar	secs[2];	/* elapsed snce client started booting */
	uchar	pad[2];
	uchar	ciaddr[4];	/* client IP address (client tells server) */
	uchar	yiaddr[4];	/* client IP address (server tells client) */
	uchar	siaddr[4];	/* server IP address */
	uchar	giaddr[4];	/* gateway IP address */
	uchar	chaddr[16];	/* client hardware address */
	char	sname[64];	/* server host name (optional) */
	char	file[128];	/* boot file name */
	char	vend[128];	/* vendor-specific goo */
};

typedef struct Netaddr Netaddr;
struct Netaddr
{
	ulong	ip;
	ushort	port;
	char	ea[Eaddrlen];
};
