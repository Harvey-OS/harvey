/*
 * common USB definitions
 */
typedef struct Ctlr Ctlr;
typedef struct Endpt Endpt;
typedef struct Udev Udev;
typedef struct Usbhost Usbhost;

enum
{
	MaxUsb = 10,	/* max # of USB Host Controller Interfaces (Usbhost*) */
	MaxUsbDev = 32,	/* max # of attached USB devs, including root hub (Udev*) */

	/* request type */
	RH2D = 0<<7,		/* output */
	RD2H = 1<<7,		/* input */
	Rstandard = 0<<5,
	Rclass	= 1<<5,
	Rvendor = 2<<5,

	Rdevice = 0,
	Rinterface = 1,
	Rendpt = 2,
	Rother = 3,
};

#define Class(csp)	((csp)&0xff)
#define Subclass(csp)	(((csp)>>8)&0xff)
#define Proto(csp)	(((csp)>>16)&0xff)
#define CSP(c, s, p)	((c) | ((s)<<8) | ((p)<<16))

/* for OHCI */
typedef struct ED ED;
struct ED {
	ulong	ctrl;
	ulong	tail;		/* transfer descriptor */
	ulong	head;
	ulong	next;
};

enum{
	Dirout,
	Dirin,
};

/*
 * device endpoint
 */
struct Endpt
{
	Ref;
	Lock;
	int	x;		/* index in Udev.ep */
	struct{		/* OHCI */
		char*	err;	/* needs to be global for unstall; fix? */
		int	xdone;
		int	xstarted;
		int	queued;	/* # of TDs queued on ED */
		Rendez	rend;
	}	dir[2];
	int	epmode;
	int	epnewmode;
	int	id;		/* hardware endpoint address */
	int	maxpkt;		/* maximum packet size (from endpoint descriptor) */
 	uchar	wdata01;	/* 0=DATA0, 1=DATA1 for output direction */
 	uchar	rdata01;	/* 0=DATA0, 1=DATA1 for input direction */
 	int	override;	/* a data command sets this and prevents
 				 * auto setting of rdata01 or wdata01
 				 */
	uchar	eof;
	ulong	csp;
	uchar	mode;		/* OREAD, OWRITE, ORDWR */
	uchar	nbuf;		/* number of buffers allowed */
	uchar	debug;
	uchar	active;		/* listed for examination by interrupts */
	int	setin;
	/* ISO is all half duplex, so need only one copy of these: */
	ulong	bw;		/* bandwidth requirement (OHCI) */
	int	hz;
	int	remain;		/* for packet size calculations */
	int	partial;	/* last iso packet may have been half full */
	Block	*bpartial;
	int	samplesz;
	int	sched;		/* schedule index; -1 if undefined or aperiodic */
	int	pollms;		/* polling interval in msec */
	int	psize;		/* (remaining) size of this packet */
	int	off;		/* offset into packet */
	/* Real-time iso stuff */
	vlong	foffset;	/* file offset (to detect seeks) */
	ulong	poffset;	/* offset of next packet to be queued */
	short	frnum;		/* frame number associated with poffset */
	int	rem;		/* remainder after rounding Hz to samples/ms */
	vlong	toffset;	/* offset associated with time */
	vlong	time;		/* time associated with offset */
	int	buffered;	/* bytes captured but unread, or written but unsent */
	/* end ISO stuff */

	Udev*	dev;		/* owning device */

	ulong	nbytes;
	ulong	nblocks;

	void	*private;

	/*
	 * all the rest could (should?) move to the driver private structure;
	 * except perhaps err.
	 */
	QLock	rlock;
	Queue*	rq;

	QLock	wlock;
	Queue*	wq;

	int	ntd;

	Endpt*	activef;	/* active endpoint list */
};

/* OHCI endpoint modes */
enum {
	Nomode,
	Ctlmode,
	Bulkmode,
	Intrmode,
	Isomode,
	Nmodes,
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

typedef enum {
	Fullspeed,	/* Don't change order, used in ehci h/w interface */
	Lowspeed,
	Highspeed,
	Nospeed,
} Speed;	/* Device speed */

/*
 * active USB device
 */
struct Udev
{
	Ref;
	Lock;
	Usbhost	*uh;
	int	x;		/* index in usbdev[] */
	int	busy;
	int	state;
	int	id;
	uchar	port;		/* port number on connecting hub */
	ulong	csp;
	ushort	vid;		/* vendor id */
	ushort	did;		/* product id */
	Speed	speed;	
	int	npt;
	Endpt*	ep[16];		/* active end points */

	Udev*	ports;		/* active ports, if hub */
	Udev*	next;		/* next device on this hub */
};

/*
 * One of these per active Host Controller Interface (HCI)
 */
struct Usbhost
{
	ISAConf;		/* hardware info */
	int	tbdf;		/* type+busno+devno+funcno */

	QLock;			/* protects namespace state */
	int	idgen;		/* version # to distinguish new connections */
	Udev*	dev[MaxUsbDev];	/* device endpoints managed by this HCI */

	void	(*init)(Usbhost*);
	void	(*interrupt)(Ureg*, void*);

	void	(*debug)(Usbhost*, char*, char*);
	void	(*portinfo)(Usbhost*, char*, char*);
	void	(*portreset)(Usbhost*, int);
	void	(*portenable)(Usbhost*, int, int);

	void	(*epalloc)(Usbhost*, Endpt*);
	void	(*epfree)(Usbhost*, Endpt*);
	void	(*epopen)(Usbhost*, Endpt*);
	void	(*epclose)(Usbhost*, Endpt*);
	void	(*epmode)(Usbhost*, Endpt*);
	void	(*epmaxpkt)(Usbhost*, Endpt*);

	long	(*read)(Usbhost*, Endpt*, void*, long, vlong);
	long	(*write)(Usbhost*, Endpt*, void*, long, vlong, int);

	void	*ctlr;

	int	tokin;
	int	tokout;
	int	toksetup;
};

extern void addusbtype(char*, int(*)(Usbhost*));
