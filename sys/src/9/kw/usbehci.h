/* override default macros from ../port/usb.h */
#undef	dprint
#undef	ddprint
#undef	deprint
#undef	ddeprint
#define dprint		if(ehcidebug)print
#define ddprint		if(ehcidebug>1)print
#define deprint		if(ehcidebug || ep->debug)print
#define ddeprint	if(ehcidebug>1 || ep->debug>1)print

typedef struct Ctlr Ctlr;
typedef struct Eopio Eopio;
typedef struct Isoio Isoio;
typedef struct Poll Poll;
typedef struct Qh Qh;
typedef struct Qtree Qtree;

#pragma incomplete Ctlr;
#pragma incomplete Ecapio;
#pragma incomplete Eopio;
#pragma incomplete Edbgio;
#pragma incomplete Isoio;
#pragma incomplete Poll;
#pragma incomplete Qh;
#pragma incomplete Qtree;

struct Poll
{
	Lock;
	Rendez;
	int	must;
	int	does;
};

struct Ctlr
{
	Rendez;			/* for waiting to async advance doorbell */
	Lock;			/* for ilock. qh lists and basic ctlr I/O */
	QLock	portlck;	/* for port resets/enable... (and doorbell) */
	int	active;		/* in use or not */
	Ecapio*	capio;		/* Capability i/o regs */
	Eopio*	opio;		/* Operational i/o regs */

	int	nframes;	/* 1024, 512, or 256 frames in the list */
	ulong*	frames;		/* periodic frame list (hw) */
	Qh*	qhs;		/* async Qh circular list for bulk/ctl */
	Qtree*	tree;		/* tree of Qhs for the periodic list */
	int	ntree;		/* number of dummy qhs in tree */
	Qh*	intrqhs;		/* list of (not dummy) qhs in tree  */
	Isoio*	iso;		/* list of active Iso I/O */
	ulong	load;
	ulong	isoload;
	int	nintr;		/* number of interrupts attended */
	int	ntdintr;	/* number of intrs. with something to do */
	int	nqhintr;	/* number of async td intrs. */
	int	nisointr;	/* number of periodic td intrs. */
	int	nreqs;
	Poll	poll;
};

/*
 * Operational registers (hw)
 */
struct Eopio
{
	ulong	cmd;		/* 00 command */
	ulong	sts;		/* 04 status */
	ulong	intr;		/* 08 interrupt enable */
	ulong	frno;		/* 0c frame index */
	ulong	seg;		/* 10 bits 63:32 of EHCI datastructs (unused) */
	ulong	frbase;		/* 14 frame list base addr, 4096-byte boundary */
	ulong	link;		/* 18 link for async list */
	uchar	d2c[0x40-0x1c];	/* 1c dummy */
	ulong	config;		/* 40 1: all ports default-routed to this HC */
	ulong	portsc[1];	/* 44 Port status and control, one per port */

	/*
	 * these are present on the Kirkwood USB controller.
	 * are they standard now?  Marvell doesn't document them publically.
	 */
	uchar	_pad0[0x64-0x48];
	ulong	otgsc;
	ulong	usbmode;
	ulong	epsetupsts;
	ulong	epprime;
	ulong	epflush;
	ulong	epsts;
	ulong	epcompl;
	ulong	epctl[6];

	/* freescale registers */
	uchar	_pad1[0x2c0-0x98];
	ulong	snoop1;
	ulong	snoop2;
	ulong	agecntthresh;
	ulong	prictl;		/* priority control */
	ulong	sictl;		/* system interface control */

	uchar	_pad2[0x3c0-0x2d4];
	ulong	ctl;
};

extern int ehcidebug;

void	ehcilinkage(Hci *hp);
void	ehcimeminit(Ctlr *ctlr);
void	ehcirun(Ctlr *ctlr, int on);
