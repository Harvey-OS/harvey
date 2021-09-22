/* override default macros from ../port/usb.h */
#undef	dprint
#undef	ddprint
#undef	deprint
#undef	ddeprint
#define dprint		if(xhcidebug)print
#define ddprint		if(xhcidebug>1)print
#define deprint		if(xhcidebug || ep->debug)print
#define ddeprint	if(xhcidebug>1 || ep->debug>1)print

typedef struct Ctlr Ctlr;
typedef struct Eopio Eopio;
typedef struct Isoio Isoio;
typedef struct Poll Poll;
typedef struct Qh Qh;
typedef struct Qtree Qtree;

#pragma incomplete Ctlr;
#pragma incomplete Eopio;
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
	Pcidev*	pcidev;
	uintptr	physio;		/* physical addr of i/o regs */
	Ecapio*	capio;		/* Capability i/o regs */
	Eopio*	opio;		/* Operational i/o regs */

	int	nframes;	/* 1024, 512, or 256 frames in the list */
	ulong*	frames;		/* periodic frame list (hw) */
	Qh*	qhs;		/* async Qh circular list for bulk/ctl */
	Qtree*	tree;		/* tree of Qhs for the periodic list */
	int	ntree;		/* number of dummy qhs in tree */
	Qh*	intrqhs;	/* list of (not dummy) qhs in tree  */
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
	ulong	pgsize;		/* 08 page size */
	ulong	frno;		/* 0c frame index */
	ulong	seg;		/* 10 bits 63:32 of XHCI datastructs (unused) */
//	ulong	frbase;		/* 14 frame list base addr, 4096-byte boundary in ehci */
	ulong	dnctl;		/* 14 dev notification */
	ulong	link;		/* 18 link for async list */
	uchar	d2c[0x400-0x1c]; /* 1c dummy */
	ulong	config;		/* 400 1: all ports default-routed to this HC */
	ulong	portsc[1];	/* 404 Port status and control, one per port */
};

extern int xhcidebug;
extern Ecapio *xhcidebugcapio;
extern int xhcidebugport;

void	xhcilinkage(Hci *hp);
void	xhcimeminit(Ctlr *ctlr);
void	xhcirun(Ctlr *ctlr, int on);
