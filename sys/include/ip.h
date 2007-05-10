#pragma	src	"/sys/src/libip"
#pragma	lib	"libip.a"

enum 
{
	IPaddrlen=	16,
	IPv4addrlen=	4,
	IPv4off=	12,
	IPllen=		4,
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
	uchar	ip[IPaddrlen];
	uchar	mask[IPaddrlen];
	uchar	net[IPaddrlen];		/* ip & mask */
	ulong	preflt;			/* preferred lifetime */
	ulong	validlt;		/* valid lifetime */
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
	uchar	sendra6;		/* on == send router adv */
	uchar	recvra6;		/* on == rcv router adv */
	int	mtu;
	ulong	pktin;
	ulong	pktout;
	ulong	errin;
	ulong	errout;
	Ipv6rp	rp;
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

uchar*	defmask(uchar*);
void	maskip(uchar*, uchar*, uchar*);
int	eipfmt(Fmt*);
int	isv4(uchar*);
ulong	parseip(uchar*, char*);
ulong	parseipmask(uchar*, char*);
char*	v4parseip(uchar*, char*);
char*	v4parsecidr(uchar*, uchar*, char*);
int	parseether(uchar*, char*);
int	myipaddr(uchar*, char*);
int	myetheraddr(uchar*, char*);
int	equivip4(uchar*, uchar*);
int	equivip6(uchar*, uchar*);

Ipifc*	readipifc(char*, Ipifc*, int);

void	hnputv(void*, uvlong);
void	hnputl(void*, uint);
void	hnputs(void*, ushort);
uvlong	nhgetv(void*);
uint	nhgetl(void*);
ushort	nhgets(void*);
ushort	ptclbsum(uchar*, int);

int	v6tov4(uchar*, uchar*);
void	v4tov6(uchar*, uchar*);

#define	ipcmp(x, y) memcmp(x, y, IPaddrlen)
#define	ipmove(x, y) memmove(x, y, IPaddrlen)

extern uchar IPv4bcast[IPaddrlen];
extern uchar IPv4bcastobs[IPaddrlen];
extern uchar IPv4allsys[IPaddrlen];
extern uchar IPv4allrouter[IPaddrlen];
extern uchar IPnoaddr[IPaddrlen];
extern uchar v4prefix[IPaddrlen];
extern uchar IPallbits[IPaddrlen];

#define CLASS(p) ((*(uchar*)(p))>>6)

#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"V"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"M"	uchar*
