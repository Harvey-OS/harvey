#pragma	src	"/sys/src/libip"
#pragma	lib	"libip.a"

int	eipconv(void*, Fconv*);
int	parseip(uchar*, char*);
int	parseether(uchar*, char*);
int	myipaddr(uchar*, char*);
int	myetheraddr(uchar*, char*);
void	maskip(uchar*, uchar*, uchar*);
int	equivip(uchar*, uchar*);

extern uchar classmask[4][4];

#define CLASS(p) ((*(uchar*)(p))>>6)

/*
 * for user level udp headers
 */
enum 
{
	Udphdrsize=	6,	/* size if a to/from user Udp header */
};

typedef struct Udphdr Udphdr;
struct Udphdr
{
	uchar	ipaddr[4];
	uchar	port[2];
};
