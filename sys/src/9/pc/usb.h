typedef struct Ctlr Ctlr;
typedef struct Endpt Endpt;
typedef struct Udev Udev;
typedef struct Usbhost Usbhost;

enum
{
	MaxUsb = 10,		/* max number of USB Host Controller Interfaces (Usbhost*) */
	MaxUsbDev = 32,	/* max number of attached USB devices, including root hub (Udev*) */

	/*
	 * USB packet definitions...
 	*/
	TokIN = 0x69,
	TokOUT = 0xE1,
	TokSETUP = 0x2D,

	/* request type */
	RH2D = 0<<7,
	RD2H = 1<<7,
	Rstandard = 0<<5,
	Rclass = 1<<5,
	Rvendor = 2<<5,
	Rdevice = 0,
	Rinterface = 1,
	Rendpt = 2,
	Rother = 3,
};

#define Class(csp)		((csp)&0xff)
#define Subclass(csp)	(((csp)>>8)&0xff)
#define Proto(csp)		(((csp)>>16)&0xff)
#define CSP(c, s, p)	((c) | ((s)<<8) | ((p)<<16))

/*
 * device endpoint
 */
struct Endpt
{
	Ref;
	Lock;
	int		x;		/* index in Udev.ep */
	int		id;		/* hardware endpoint address */
	int		maxpkt;	/* maximum packet size (from endpoint descriptor) */
	int		data01;	/* 0=DATA0, 1=DATA1 */
	uchar	eof;
	ulong	csp;
	uchar	mode;	/* OREAD, OWRITE, ORDWR */
	uchar	nbuf;	/* number of buffers allowed */
	uchar	iso;
	uchar	debug;
	uchar	active;	/* listed for examination by interrupts */
	int		setin;
	/* ISO related: */
	int		hz;
	int		remain;	/* for packet size calculations */
	int		samplesz;
	int		sched;	/* schedule index; -1 if undefined or aperiodic */
	int		pollms;	/* polling interval in msec */
	int		psize;	/* (remaining) size of this packet */
	int		off;		/* offset into packet */
	/* Real-time iso stuff */
	ulong	foffset;	/* file offset (to detect seeks) */
	ulong	poffset;	/* offset of next packet to be queued */
	ulong	toffset;	/* offset associated with time */
	vlong	time;		/* timeassociated with offset */
	int		buffered;	/* bytes captured but unread, or written but unsent */
	/* end ISO stuff */

	Udev*	dev;	/* owning device */

	ulong	nbytes;
	ulong	nblocks;

	void	*private;

	// all the rest could (should?) move to the driver private structure; except perhaps err
	QLock	rlock;
	Rendez	rr;
	Queue*	rq;
	QLock	wlock;
	Rendez	wr;
	Queue*	wq;

	int		ntd;
	char*	err;		// needs to be global for unstall; fix?

	Endpt*	activef;	/* active endpoint list */
};

/* device parameters */
enum
{
	/* Udev.state */
	Disabled = 0,
	Attached,
	Enabled,
	Assigned,
	Configured,

	/* Udev.class */
	Noclass = 0,
	Hubclass = 9,
};

/*
 * active USB device
 */
struct Udev
{
	Ref;
	Lock;
	Usbhost	*uh;
	int		x;		/* index in usbdev[] */
	int		busy;
	int		state;
	int		id;
	uchar	port;		/* port number on connecting hub */
	ulong	csp;
	ushort	vid;		/* vendor id */
	ushort	did;		/* product id */
	int		ls;
	int		npt;
	Endpt*	ep[16];	/* active end points */
	Udev*	ports;	/* active ports, if hub */
	Udev*	next;		/* next device on this hub */
};

/*
 * One of these per active Host Controller Interface (HCI)
 */
struct Usbhost
{
	ISAConf;					/* hardware info */
	int	tbdf;					/* type+busno+devno+funcno */

	QLock;					/* protects namespace state */
	int		idgen;			/* version number to distinguish new connections */
	Udev*	dev[MaxUsbDev];	/* device endpoints managed by this HCI */

	void	(*init)(Usbhost*);
	void	(*interrupt)(Ureg*, void*);

	void	(*portinfo)(Usbhost*, char*, char*);
	void	(*portreset)(Usbhost*, int);
	void	(*portenable)(Usbhost*, int, int);

	void	(*epalloc)(Usbhost*, Endpt*);
	void	(*epfree)(Usbhost*, Endpt*);
	void	(*epopen)(Usbhost*, Endpt*);
	void	(*epclose)(Usbhost*, Endpt*);
	void	(*epmode)(Usbhost*, Endpt*);

	long	(*read)(Usbhost*, Endpt*, void*, long, vlong);
	long	(*write)(Usbhost*, Endpt*, void*, long, vlong, int);

	void	*ctlr;
};

extern void addusbtype(char*, int(*)(Usbhost*));
