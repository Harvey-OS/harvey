/*
 * All the goo for PC ethernet cards.
 */
typedef struct Board Board;
typedef struct RingBuf RingBuf;
typedef struct Type Type;
typedef struct Ctlr Ctlr;

/*
 * Hardware interface.
 */
struct Board {
	int	(*reset)(Ctlr*);
	void	(*init)(Ctlr*);
	void	(*attach)(Ctlr*);
	void	(*mode)(Ctlr*, int);

	void	(*receive)(Ctlr*);
	void	(*transmit)(Ctlr*);
	void	(*intr)(Ctlr*);
	void	(*watch)(Ctlr*);

	ulong	io;			/* interface I/O base address */
	uchar	irq;			/* interrupt level */
	uchar	bit16;			/* true if a 16 bit interface */

	uchar	ram;			/* true if interface has shared memory */
	ulong	ramstart;		/* interface shared memory address start */
	ulong	ramstop;		/* interface shared memory address end */

	ulong	dp8390;			/* I/O address of 8390 (if any) */
	ulong	data;			/* I/O data port if no shared memory */
	uchar	tstart;			/* 8390 ring addresses */
	uchar	pstart;
	uchar	pstop;
};

/*
 * Software ring buffer.
 */
struct RingBuf {
	uchar	owner;
	uchar	busy;
	ushort	len;
	uchar	pkt[sizeof(Etherpkt)];
};

enum {
	Host		= 0,	/* buffer owned by host */
	Interface	= 1,	/* buffer owned by interface */

	Nrb		= 16,	/* default number of receive buffers */
	Ntb		= 4,	/* default number of transmit buffers */
};

/*
 * One per ethernet packet type.
 */
struct Type {
	QLock;
	Netprot;		/* stat info */
	int	type;		/* ethernet type */
	int	prom;		/* promiscuous mode */
	Queue	*q;
	int	inuse;
	Ctlr	*ctlr;

	Rendez	cr;		/* rendezvous for close */
	Type	*clist;		/* close list */
};

enum {
	NType		= 9,	/* types/interface */
};

/*
 * Software controller.
 */
struct Ctlr {
	QLock;

	Board	*board;
	int	present;

	ushort	nrb;		/* number of software receive buffers */
	ushort	ntb;		/* number of software transmit buffers */
	RingBuf *rb;		/* software receive buffers */
	RingBuf *tb;		/* software transmit buffers */

	uchar	ea[6];		/* ethernet address */
	uchar	ba[6];		/* broadcast address */

	Rendez	rr;		/* rendezvous for a receive buffer */
	ushort	rh;		/* first receive buffer belonging to host */
	ushort	ri;		/* first receive buffer belonging to interface */	

	Rendez	tr;		/* rendezvous for a transmit buffer */
	QLock	tlock;		/* semaphore on th */
	ushort	th;		/* first transmit buffer belonging to host */	
	ushort	ti;		/* first transmit buffer belonging to interface */	

	Type	type[NType];
	int	all;		/* number of channels listening to all packets */
	Type	*clist;		/* channels waiting to close */
	Lock	clock;		/* lock for clist */
	int	prom;		/* number of promiscuous channels */
	int	kproc;		/* true if kproc started */
	char	name[NAMELEN];	/* name of kproc */
	Network	net;

	Queue	lbq;		/* software loopback packet queue */

	int	inpackets;
	int	outpackets;
	int	crcs;		/* input crc errors */
	int	oerrs;		/* output errors */
	int	frames;		/* framing errors */
	int	overflows;	/* packet overflows */
	int	buffs;		/* buffering errors */
};

#define NEXT(x, l)	(((x)+1)%(l))
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

/*
 * The Western Digital 8003 and 8013 series, the NE2000
 * and the 3Com 503 all use the DP8390 Network Interface
 * Chip.
 */
extern void dp8390reset(Ctlr*);
extern void dp8390attach(Ctlr*);
extern void dp8390mode(Ctlr*, int);
extern void dp8390setea(Ctlr*);
extern void *dp8390read(Ctlr*, void*, ulong, ulong);
extern void dp8390receive(Ctlr*);
extern void dp8390transmit(Ctlr*);
extern void dp8390intr(Ctlr*);

enum {
	Dp8390BufSz	= 256,		/* hardware ring buffer size */
};
