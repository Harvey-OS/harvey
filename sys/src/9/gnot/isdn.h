typedef struct Isdn	Isdn;
typedef struct Unite	Unite;
typedef struct Hifi	Hifi;

/*
struct Isdndev {
	uchar	asr;
	uchar	data;
};
*/

#define	ISDNDEV0	((uchar *)&PORT[52])	/* the first one */

#define	Bit(n)		(1<<(n))

#define	Bits(m,n)	(((1<<((n)-(m)+1))-1)<<(m))

enum Isdn_asr {		/* Board status */
	Uint	= Bit(0),
	Hint0	= Bit(1),
	Hint1	= Bit(2),
	DRCODEC = Bit(6),	/* enable codec receiver */
	DXCODEC = Bit(7)	/* enable codec transmitter */
};

/*
 * Board addresses
 */

#define	unite	((Unite *)(0*Bit(4)))
#define	hifi0	((Hifi *) (2*Bit(4)))
#define	hifi1	((Hifi *) (3*Bit(4)))

struct Unite {
	uchar	config;	/* Chip/Hardware Configuration Register */
	uchar	listat;	/* Line Interface Status */
	uchar	tctl;	/* Transmitter Control Register */
	uchar	data;	/* D-channel data register */
	uchar	hcr;	/* Hardware Configuration Register */
	uchar	rstat;	/* HDLC Receiver Status */
	uchar	itl;	/* Interrupt Trigger Levels for D-Channel Queues */
	uchar	mfstat;	/* Multiframe Status and S-Channel Queues */
	uchar	stictl;	/* S/T Interface control */
	uchar	imask;	/* Interrupt masks */
	uchar	istat;	/* Interrupt Status */
	uchar	qstat;	/* Q-Channel Data and Status */
	uchar	lctl;	/* Loopback and Transmit 1's Control */
	uchar	timctl;	/* Timer Configuration Control */
	uchar	tstat;	/* Transmitter Queue Status */
	uchar	reset;	/* Software Resets */
};

struct Hifi {
	uchar	config;	/* Chip Configuration Register */
	uchar	tctl;	/* Transmitter Control Register */
	uchar	tstat;	/* Transmitter Status Register */
	uchar	data;	/* r3 is the Data Byte Register. */
	uchar	rstat;	/* Receiver Status Register */
	uchar	rctl;	/* Receiver Control Register */
	uchar	opreg;	/* Operation Register */
	uchar	ttsctl;	/* Transmitter Time Slot Control Register */
	uchar	rtsctl;	/* Receiver Time Slot Control Register */
	uchar	bofctl;	/* Bit Offset Control Register */
	uchar	tofctl;	/* Transmitter Time Slot Offset Control Register */
	uchar	rof_tm;	/* Rcvr Time Slot Offset/Trans. Mode Control Register */
	uchar	rmask;	/* Receiver Mask/Character Register. */
	uchar	tmask;	/* Transmitter Mask/Idle Char. Register. */
	uchar	imask;	/* Interrupt Mask Register */
	uchar	istat;	/* Interrupt Status Register */
};

/*
 *	usage: SET(unite->chcr, C2pol);
 */

#define	SETADDR(addr)	(*(ip->asr) = ip->venval | ((int)(addr)))

#define	GET(lval)	(SETADDR(&(lval)), *(ip->data))

#define	SET(lval, rval)	(SETADDR(&(lval)), *(ip->data)=(rval))

#define	NBMASK	0x0f
#define	NB	(NBMASK+1)
#define	NEXT(a)	(((a)+1)&NBMASK)

struct Isdn
{
	QLock;			/* access to struct */
	QLock	kctl;		/* kproc start/stop */
	Rendez	kctlr;
	int	kstart;		/* set if kernel process started */
	Rendez	kr;		/* kernel process sleep/wakeup */
	Rendez	ar;		/* wait for active link (INFO 4) */

	int	pri;		/* priority at time of lock(&dev) */
	Lock	dev;		/* access to device */
	uchar	venval;
	uchar *	asr;
	uchar *	data;
	int	hangup;		/* hangup pending */
	uchar	listat;		/* previous line interface status */
	uchar	tctl;		/* transmitter control */
	uchar	imask;		/* interrupt mask */
	uchar	stictl;		/* S/T interface control and b1xb2 bit */
	uchar	lctl;		/* loopback control */
	uchar	devaddr;	/* debug access */

	Block *	inb[NB];	/* input buffer */
	int	ri;		/* input read pointer */
	int	wi;		/* input write pointer */
	Queue *	rq;		/* read queue */

	QLock	wlock;		/* write's are atomic */
	Block *	outb[NB];	/* output buffer */
	int	so;		/* output scavenge pointer */
	int	ro;		/* output read pointer */
	int	wo;		/* output write pointer */
	Block *	out;		/* current output block */
	Rendez	tr;		/* if transmitter must wait */

	/* statistics */

	ulong	inchars;
	ulong	badcrc;
	ulong	abort;
	ulong	overrun;
	ulong	badcount;
	ulong	overflow;
	ulong	toolong;
	ulong	underrun;
	ulong	outchars;
};

extern Isdn *	isdndev;
extern void	isdnlock(Isdn*);
extern int	isdnpeek(Isdn*, void*);
extern void	isdnpoke(Isdn*, void*, int);
extern void	isdnunlock(Isdn*);
extern int	spl2(void);
