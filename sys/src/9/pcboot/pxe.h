/* from <ip.h> */

enum
{
	ETHER_HDR	= 14,
	ET_IP		= 0x800,

	IP_VER		= 0x40,
	IP_HLEN		= 0x05,
 	IP_UDPPROTO	= 17,

	UDP_EHSIZE	= 22,
	UDP_PHDRSIZE	= 12,
	UDP_HDRSIZE	= 20,

	Tftphdrsz	= 4,
	Maxsegsize	= 2048,

	BPportsrc	= 68,
	BPportdst	= 67,
	Bootrequest 	= 1,
	Bootreply   	= 2,

	/* lengths of some bootp fields */
	Maxhwlen=	16,
	Maxfilelen=	128,
	Maxoptlen=	312-4,

	/* bootp option types */
	OBend=			255,
	OBpad=			0,
	OBmask=			1,
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

/*
 * from 9load
 */
typedef struct Bootp Bootp;
struct Bootp
{
	uchar	op;		/* opcode */
	uchar	htype;		/* hardware type */
	uchar	hlen;		/* hardware address len */
	uchar	hops;		/* hops */
	uchar	xid[4];		/* a random number */
	uchar	secs[2];	/* elapsed since client started booting */
	uchar	flags[2];	/* unused in bootp, flags in dhcp */
	uchar	ciaddr[4];	/* client IP address (client tells server) */
	uchar	yiaddr[4];	/* client IP address (server tells client) */
	uchar	siaddr[4];	/* server IP address */
	uchar	giaddr[4];	/* gateway IP address */
	uchar	chaddr[16];	/* client hardware address */
	char	sname[64];	/* server host name (optional) */
	char	file[128];	/* boot file name */

//	char	vend[128];	/* vendor-specific goo */
	uchar	optmagic[4];
	uchar	optdata[Maxoptlen];
};

typedef struct Pxenetaddr Pxenetaddr;
struct Pxenetaddr
{
	uchar	ip[IPaddrlen];
	ushort	port;
};

typedef struct Openeth Openeth;
struct Openeth {
	/* names */
	int	ctlrno;
	char	ethname[16];	/* ether%d */
	char	netethname[32];	/* /net/ether%d */
	char	filename[128];	/* from bootp, for tftp */

	Chan	*ifcctl;	/* /net/ipifc/clone */
	Chan	*ethctl;	/* /net/etherN/0/ctl, for promiscuous mode */

	/* udp connection */
	Chan	*udpctl;
	Chan	*udpdata;
	Pxenetaddr *netaddr;
	int	rxactive;
};

typedef struct Tftp Tftp;
struct Tftp {
	uchar	header[Tftphdrsz];
	uchar	data[Maxsegsize];
};

extern int chatty, progress, segsize;

int	announce(Openeth *oe, char *port);
void	closeudp(Openeth *oe);
char	*filetoload(Openeth *oe, char *file, Bootp *rep);
int	udprecv(Openeth *oe, Pxenetaddr *a, void *data, int dlen);
void	udpsend(Openeth *oe, Pxenetaddr *a, void *data, int dlen);
