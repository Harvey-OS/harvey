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

struct Ipifc
{
	char	dev[64];
	uchar	ip[IPaddrlen];
	uchar	mask[IPaddrlen];
	uchar	net[IPaddrlen];		/* ip & mask */
	int	mtu;
	int	index;			/* number of interface in ipifc dir */
	Ipifc	*next;
};

/*
 *  user level udp headers
 */
enum 
{
	Udphdrsize=	36,	/* size of a Udphdr */
};

typedef struct Udphdr Udphdr;
struct Udphdr
{
	uchar	raddr[IPaddrlen];	/* remote address and port */
	uchar	laddr[IPaddrlen];	/* local address and port */
	uchar	rport[2];
	uchar	lport[2];
};

uchar*	defmask(uchar*);
void	maskip(uchar*, uchar*, uchar*);
int	eipconv(va_list*, Fconv*);
int	isv4(uchar*);
ulong	parseip(uchar*, char*);
ulong	parseipmask(uchar*, char*);
char*	v4parseip(uchar*, char*);
char*	v4parsecidr(uchar*, uchar*, char*);
int	parseether(uchar*, char*);
int	myipaddr(uchar*, char*);
int	myetheraddr(uchar*, char*);
int	equivip(uchar*, uchar*);

Ipifc*	readipifc(char*, Ipifc*);

void	hnputl(void*, uint);
void	hnputs(void*, ushort);
uint	nhgetl(void*);
ushort	nhgets(void*);

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
