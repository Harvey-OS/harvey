typedef struct Etherpkt	Etherpkt;
typedef struct Netaddr	Netaddr;
typedef struct Netfile	Netfile;
typedef struct Netif	Netif;

enum
{
	Nmaxaddr=	64,
	Nmhash=		31,

	Ncloneqid=	1,
	Naddrqid,
	N2ndqid,
	N3rdqid,
	Ndataqid,
	Nctlqid,
	Nstatqid,
	Ntypeqid,
	Nifstatqid,
};

/*
 *  Macros to manage Qid's used for multiplexed devices
 */
#define NETTYPE(x)	(((ulong)x)&0x1f)
#define NETID(x)	((((ulong)x))>>5)
#define NETQID(i,t)	((((ulong)i)<<5)|(t))

/*
 *  one per multiplexed connection
 */
struct Netfile
{
	QLock;

	int	inuse;
	ulong	mode;
	char	owner[KNAMELEN];

	int	type;			/* multiplexor type */
	int	prom;			/* promiscuous mode */
	int	scan;			/* base station scanning interval */
	int	bridge;			/* bridge mode */
	int	headersonly;		/* headers only - no data */
	uchar	maddr[8];		/* bitmask of multicast addresses requested */
	int	nmaddr;			/* number of multicast addresses */

	Queue	*in;			/* input buffer */
};

/*
 *  a network address
 */
struct Netaddr
{
	Netaddr	*next;		/* allocation chain */
	Netaddr	*hnext;
	uchar	addr[Nmaxaddr];
	int	ref;
};

/*
 *  a network interface
 */
struct Netif
{
	QLock;

	/* multiplexing */
	char	name[KNAMELEN];		/* for top level directory */
	int	nfile;			/* max number of Netfiles */
	Netfile	**f;

	/* about net */
	int	limit;			/* flow control */
	int	alen;			/* address length */
	int	mbps;			/* megabits per sec */
	int	link;			/* link status */
	uchar	addr[Nmaxaddr];
	uchar	bcast[Nmaxaddr];
	Netaddr	*maddr;			/* known multicast addresses */
	int	nmaddr;			/* number of known multicast addresses */
	Netaddr *mhash[Nmhash];		/* hash table of multicast addresses */
	int	prom;			/* number of promiscuous opens */
	int	scan;			/* number of base station scanners */
	int	all;			/* number of -1 multiplexors */

	/* statistics */
	int	misses;
	int	inpackets;
	int	outpackets;
	int	crcs;		/* input crc errors */
	int	oerrs;		/* output errors */
	int	frames;		/* framing errors */
	int	overflows;	/* packet overflows */
	int	buffs;		/* buffering errors */
	int	soverflows;	/* software overflow */

	/* routines for touching the hardware */
	void	*arg;
	void	(*promiscuous)(void*, int);
	void	(*multicast)(void*, uchar*, int);
	void	(*scanbs)(void*, uint);	/* scan for base stations */
};

void	netifinit(Netif*, char*, int, ulong);
Walkqid*	netifwalk(Netif*, Chan*, Chan*, char **, int);
Chan*	netifopen(Netif*, Chan*, int);
void	netifclose(Netif*, Chan*);
long	netifread(Netif*, Chan*, void*, long, ulong);
Block*	netifbread(Netif*, Chan*, long, ulong);
long	netifwrite(Netif*, Chan*, void*, long);
int	netifwstat(Netif*, Chan*, uchar*, int);
int	netifstat(Netif*, Chan*, uchar*, int);
int	activemulti(Netif*, uchar*, int);

/*
 *  Ethernet specific
 */
enum
{
	Eaddrlen=	6,
	ETHERMINTU =	60,		/* minimum transmit size */
	ETHERMAXTU =	1514,		/* maximum transmit size */
	ETHERHDRSIZE =	14,		/* size of an ethernet header */
};

struct Etherpkt
{
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
};
