/*
 * All the goo for PC ethernet cards.
 */
typedef struct Card Card;
typedef struct RingBuf RingBuf;
typedef struct Type Type;
typedef struct Ctlr Ctlr;

/*
 * Hardware interface.
 */
struct Card {
	ISAConf;

	int	(*reset)(Ctlr*);
	void	(*attach)(Ctlr*);

	void	*(*read)(Ctlr*, void*, ulong, ulong);
	void	*(*write)(Ctlr*, ulong, void*, ulong);

	void	(*receive)(Ctlr*);
	void	(*transmit)(Ctlr*);
	void	(*intr)(Ureg*, Ctlr*);
	void	(*overflow)(Ctlr*);

	uchar	bit16;			/* true if a 16 bit interface */
	uchar	ram;			/* true if card has shared memory */

	ulong	dp8390;			/* I/O address of 8390 (if any) */
	ulong	data;			/* I/O data port if no shared memory */
	uchar	nxtpkt;			/* software bndry */
	uchar	tstart;			/* 8390 ring addresses */
	uchar	pstart;
	uchar	pstop;
};

/*
 * Software ring buffer.
 */
struct RingBuf {
	uchar	owner;
	uchar	busy;			/* unused */
	ushort	len;
	uchar	pkt[sizeof(Etherpkt)];
};

enum {
	Host		= 0,		/* buffer owned by host */
	Interface	= 1,		/* buffer owned by card */

	Nrb		= 16,		/* default number of receive buffers */
	Ntb		= 1,		/* default number of transmit buffers */
};

/*
 * Software controller.
 */
struct Ctlr {
	Card	card;			/* hardware info */
	int	ctlrno;
	char	iname[NAMELEN];
	char	oname[NAMELEN];
	int	present;

	ushort	nrb;			/* number of software receive buffers */
	ushort	ntb;			/* number of software transmit buffers */
	RingBuf *rb;			/* software receive buffers */
	RingBuf *tb;			/* software transmit buffers */

	ushort	rh;			/* first receive buffer belonging to host */
	ushort	ri;			/* first receive buffer belonging to card */	

	ushort	th;			/* first transmit buffer belonging to host */	
	ushort	ti;			/* first transmit buffer belonging to card */
	int	tbusy;			/* transmitter is busy */	

	int	inpackets;
	int	outpackets;
	int	crcs;			/* input crc errors */
	int	oerrs;			/* output errors */
	int	frames;			/* framing errors */
	int	overflows;		/* packet overflows */
	int	buffs;			/* buffering errors */
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
extern void dp8390getea(Ctlr*);
extern void dp8390setea(Ctlr*);
extern void *dp8390read(Ctlr*, void*, ulong, ulong);
extern void *dp8390write(Ctlr*, ulong, void*, ulong);
extern void dp8390receive(Ctlr*);
extern void dp8390transmit(Ctlr*);
extern void dp8390intr(Ureg*, Ctlr*);
extern void dp8390overflow(Ctlr*);

/*
 * The DP8390 needs some time between successive 
 * chip selects, so we need special I/O routines.
 * See l.s.
 * These routines only need to be used for accessing
 * the chip registers. Data I/O, either through
 * remote DMA or shared memory, can use the normal
 * routines.
 */
extern int dp8390inb(ulong);
extern void dp8390outb(ulong, uchar);

enum {
	Dp8390BufSz	= 256,		/* hardware ring buffer size */
};

extern int ne2000reset(Ctlr*);
extern int ne3210reset(Ctlr*);
extern int tcm509reset(Ctlr*);
extern int wd8003reset(Ctlr*);
