enum 
{
	IPaddrlen=	16,
	IPv4addrlen=	4,
	IPv4off=	12,
};

uchar*	defmask(uchar*);
void	maskip(uchar*, uchar*, uchar*);
int	eipfmt(Fmt*);
int	isv4(uchar*);
vlong	parseip(uchar*, char*);
vlong	parseipmask(uchar*, char*);
char*	v4parseip(uchar*, char*);
char*	v4parsecidr(uchar*, uchar*, char*);

void	hnputv(void*, uvlong);
void	hnputl(void*, uint);
void	hnputs(void*, ushort);
uvlong	nhgetv(void*);
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
