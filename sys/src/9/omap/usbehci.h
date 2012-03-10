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
	ulong	intr;		/* 08 interrupt enable */
	ulong	frno;		/* 0c frame index */
	ulong	seg;		/* 10 bits 63:32 of EHCI datastructs (unused) */
	ulong	frbase;		/* 14 frame list base addr, 4096-byte boundary */
	ulong	link;		/* 18 link for async list */
	uchar	d2c[0x40-0x1c];	/* 1c dummy */
	ulong	config;		/* 40 1: all ports default-routed to this HC */
	ulong	portsc[3];	/* 44 Port status and control, one per port */

	/* defined for omap35 ehci at least */
	uchar	_pad0[0x80 - 0x50];
	ulong	insn[6];	/* implementation-specific */
};

typedef struct Uhh Uhh;
struct Uhh {
	ulong	revision;	/* ro */
	uchar	_pad0[0x10-0x4];
	ulong	sysconfig;
	ulong	sysstatus;	/* ro */

	uchar	_pad1[0x40-0x18];
	ulong	hostconfig;
	ulong	debug_csr;
};

enum {
	/* hostconfig bits */
	P1ulpi_bypass = 1<<0,	/* utmi if set; else ulpi */
};

extern Ecapio *ehcidebugcapio;
extern int ehcidebugport;

extern int ehcidebug;

void	ehcilinkage(Hci *hp);
void	ehcimeminit(Ctlr *ctlr);
void	ehcirun(Ctlr *ctlr, int on);
