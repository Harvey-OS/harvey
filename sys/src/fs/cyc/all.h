#define	BITBOTCH	256	/* remove with BIT3 */
#define	nelem(x)	(sizeof(x)/sizeof(x[0]))
#define	XMIT(x)		{ SQL->txfifo = (x); }
#define	FIFOE		((SQL->fifocsr&Rcv_ne) == 0)
#define FIFOXMTBLK	((SQL->fifocsr&Xmit_flag) == 0)
#define FIFORCVBLK	((SQL->fifocsr&Rcv_flag) == 0)
#define	RECV(x)		{ (x) = SQL->rxfifo; }
#define	DEBUG		if(0)print
#define	NEXTPING(x)	((x)+1)
#define	SQL		((Taxi*)0xc0000110)
#define VIC		((Vic*)0xa0000000)

enum
{
	NBUF		= 200,			/* total buffers */
	HPP		= sizeof(ulong),	/* host address ++ */
	MEDIAN		= 1000,			/* somewhere between large and small */
	MAXXMIT		= 8192+128+BITBOTCH,	/* p9 protocol */
	SMALLSIZE	= 70,
	CHUNK		= 64,			/* word count for burst on vme/taxi */
	ALIGNQM		= 16,			/* bmove align */
};

typedef	struct	Mesg	Mesg;

struct	Mesg
{
	Mesg*	next;
	int	type;
	long	bsize;
	ulong	csum;
	ulong	haddr;
	ulong	bufp;
};

struct
{
	ulong	pingvalue;	/* value to reply to Uping */
	ulong	pingcount;
	int	pingpend;	/* transmit a ping */

	Mesg*	sbuflist;	/* list of small Ubuf requests */
	Mesg*	lbuflist;	/* list of large Ubuf requests */
	Mesg*	wbuflist;	/* list of Uwrite requests */
	Mesg*	freebuflist;	/* list of free buffers */
	Mesg	mesg[NBUF];

	/*
	 * cyc->vme dma parameters
	 */
	ulong	fifout;
	ulong	vmeaddr;
	ulong	vmecount;
	ulong	vmesum;
	struct
	{
		int	type;
		ulong	haddr;
		ulong	s;
		ulong	u;
		ulong	n;
	} vmeblk;
} b;
