typedef struct Ether Ether;
typedef struct Etherops Etherops;
typedef struct Conn Conn;
typedef struct Cinfo Cinfo;
typedef struct Buf Buf;
typedef struct Etherpkt Etherpkt;

enum
{
	/* controller ids */
	Cdc = 0,
	A8817x,		/* Asis */
	A88178,
	A88179,
	A88772,
	S95xx,		/* SMSC */

	Eaddrlen = 6,
	Epktlen = 1514,
	Ehdrsize = 2*Eaddrlen + 2,

	Maxpkt	= 2000,	/* no jumbo packets here */
	Nconns	= 8,	/* max number of connections */
	Nbufs	= 32,	/* max number of buffers */
	Scether = 6,	/* ethernet cdc subclass */
	Fnheader = 0,	/* Functions */
	Fnunion = 6,
	Fnether = 15,

	Cdcunion	= 6,	/* CDC Union descriptor subtype */
};

struct Buf
{
	int	type;
	int	ndata;
	uchar*	rp;
	uchar	data[Hdrsize+Maxpkt];
};

struct Conn
{
	Ref;			/* one per file in use */
	int	nb;
	int	type;
	int	headersonly;
	int	prom;
	Channel*rc;		/* [2] of Buf* */
};

struct Etherops
{
	int	(*init)(Ether*, int *epin, int *epout);
	long	(*bread)(Ether*, Buf*);
	long	(*bwrite)(Ether*, Buf*);
	int	(*ctl)(Ether*, char*);
	int	(*promiscuous)(Ether*, int);
	int	(*multicast)(Ether*, uchar*, int);
	char*	(*seprintstats)(char*, char*, Ether*);
	void	(*free)(Ether*);
	int	bufsize;
	char	*name;
	void*	aux;
};

struct Ether
{
	QLock;
	QLock	wlck;			/* write one at a time */
	int	epinid;			/* epin address */
	int	epoutid;			/* epout address */
	Dev*	dev;
	Dev*	epin;
	Dev*	epout;
	int	cid;			/* ctlr id */
	int	phy;			/* phy id */
	Ref	prom;			/* nb. of promiscuous conns */
	int	exiting;			/* shutting down */
	int	wrexited;			/* write process died */
	uchar	addr[Eaddrlen];		/* mac */
	int	nconns;			/* nb. of entries used in... */
	Conn*	conns[Nconns];		/* connections */
	int	nabufs;			/* nb. of allocated buffers */
	int	nbufs;			/* nb. of buffers in use */
	int	nblock;			/* nonblocking (output)? */
	long	nin;
	long	nout;
	long	nierrs;
	long	noerrs;
	int	mbps;
	int	nmcasts;
	Channel*rc;			/* read channel (of Buf*) */
	Channel*wc;			/* write channel (of Buf*) */
	Channel*bc;			/* free buf. chan. (of Buf*) */
	Etherops;
	Usbfs	fs;
};

struct Cinfo
{
	int vid;		/* usb vendor id */
	int did;		/* usb device/product id */
	int cid;		/* controller id assigned by us */
};

struct Etherpkt
{
	uchar d[Eaddrlen];
	uchar s[Eaddrlen];
	uchar type[2];
	uchar data[1500];
};

int	ethermain(Dev *dev, int argc, char **argv);
int	asixreset(Ether*);
int smscreset(Ether*);
int	cdcreset(Ether*);
int	parseaddr(uchar *m, char *s);
void	dumpframe(char *tag, void *p, int n);

extern Cinfo cinfo[];
extern int etherdebug;

#define	deprint	if(etherdebug)fprint
