typedef struct RingBuf {
	uchar	owner;
	uchar	unused;
	ushort	len;
	uchar	pkt[sizeof(Etherpkt)];
} RingBuf;

enum {
	Host		= 0,		/* buffer owned by host */
	Interface	= 1,		/* buffer owned by card */

	Nrb		= 32,		/* default number of receive buffers */
	Ntb		= 8,		/* default number of transmit buffers */
};

/*
 *  a parsed .ini line
 */
#define ISAOPTLEN	32
#define NISAOPT		8
#define NAMELEN		28

typedef struct  ISAConf {
	char	type[NAMELEN];
	ulong	port;
//	ulong	irq;
	ulong	mem;
	ulong	size;
	uchar	ea[6];

	int	nopt;
	char	opt[NISAOPT][ISAOPTLEN];
} ISAConf;

#define	CONFADDR	(0x2200)		/* above ppc vectors */
#define BOOTLINE	((char*)CONFADDR)

typedef struct Ether Ether;
struct Ether {
	ISAConf;			/* hardware info */
	ushort	ctlrno;
	ushort	state;			/* 0: unfound, 1: found, 2: attaching */

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*transmit)(Ether*);
	int	(*interrupt)(ulong bit);
	void	(*detach)(Ether*);
	void	*ctlr;

	ushort	nrb;			/* number of software receive buffers */
	ushort	ntb;			/* number of software transmit buffers */
	RingBuf *rb;			/* software receive buffers */
	RingBuf *tb;			/* software transmit buffers */

	ushort	rh;			/* first receive buffer belonging to host */
	ushort	ri;			/* first receive buffer belonging to card */	

	ushort	th;			/* first transmit buffer belonging to host */	
	ushort	ti;			/* first transmit buffer belonging to card */
	ushort	tbusy;			/* transmitter is busy */
	ushort	mbps;			/* zero means link down */	
};

extern void etherrloop(Ether*, Etherpkt*, long);
extern void addethercard(char*, int(*)(Ether*));

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
