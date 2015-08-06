/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
typedef struct Ecapio Ecapio;
typedef struct Edbgio Edbgio;
typedef struct Eopio Eopio;
typedef struct Isoio Isoio;
typedef struct Poll Poll;
typedef struct Qh Qh;
typedef struct Qtree Qtree;

#pragma incomplete Ctlr;
#pragma incomplete Ecapio;
#pragma incomplete Edbgio;
#pragma incomplete Eopio;
#pragma incomplete Isoio;
#pragma incomplete Poll;
#pragma incomplete Qh;
#pragma incomplete Qtree;

/*
 * EHCI interface registers and bits
 */
enum
{
#ifdef WTF
	/* Ecapio->parms reg. */
	Cnports		= 0xF,		/* nport bits */
	Cdbgportshift	= 20,		/* debug port */
	Cdbgportmask	= 0xF,

	/* Ecapio->capparms bits */
	C64		= 1<<0,		/* 64-bits */
	Cpfl		= 1<<1,	/* program'ble frame list: can be <1024 */
	Casp		= 1<<2,		/* asynch. sched. park */
	Ceecpshift	= 8,		/* extended capabilities ptr. */
	Ceecpmask	= (1<<8) - 1,

	Clegacy		= 1,		/* legacy support cap. id */
	CLbiossem	= 2,		/* legacy cap. bios sem. */
	CLossem		= 3,		/* legacy cap. os sem */
	CLcontrol	= 4,		/* legacy support control & status */

	/* typed links  */
	Lterm		= 1,
	Litd		= 0<<1,
	Lqh		= 1<<1,
	Lsitd		= 2<<1,
	Lfstn		= 3<<1,		/* we don't use these */

	/* Cmd reg. */
	Cstop		= 0x00000,	/* stop running */
	Crun		= 0x00001,	/* start operation */
	Chcreset	= 0x00002,	/* host controller reset */
	Cflsmask	= 0x0000C,	/* frame list size bits */
	Cfls1024	= 0x00000,	/* frame list size 1024 */
	Cfls512		= 0x00004,	/* frame list size 512 frames */
	Cfls256		= 0x00008,	/* frame list size 256 frames */
	Cpse		= 0x00010,	/* periodic sched. enable */
	Case		= 0x00020,	/* async sched. enable */
	Ciasync		= 0x00040,	/* interrupt on async advance doorbell */
	Citcmask	= 0xff << 16,
#endif
	Citc1		= 0x10000,	/* interrupt threshold ctl. 1 µframe */
	Citc4		= 0x40000,	/* same. 2 µframes */
	/* ... */
	Citc8		= 0x80000,	/* same. 8 µframes (can go up to 64) */
#ifdef WTF
	/* Sts reg. */
	Sasyncss	= 0x08000,	/* aync schedule status */
	Speriodss	= 0x04000,	/* periodic schedule status */
	Srecl		= 0x02000,	/* reclamnation (empty async sched.) */
	Shalted		= 0x01000,	/* h.c. is halted */
	Sasync		= 0x00020,	/* interrupt on async advance */
	Sherr		= 0x00010,	/* host system error */
	Sfrroll		= 0x00008,	/* frame list roll over */
	Sportchg	= 0x00004,	/* port change detect */
	Serrintr	= 0x00002,		/* error interrupt */
	Sintr		= 0x00001,	/* interrupt */
	Sintrs		= 0x0003F,	/* interrupts status */

	/* Intr reg. */
	Iusb		= 0x01,		/* intr. on usb */
	Ierr		= 0x02,		/* intr. on usb error */
	Iportchg	= 0x04,		/* intr. on port change */
	Ifrroll		= 0x08,		/* intr. on frlist roll over */
	Ihcerr		= 0x10,		/* intr. on host error */
	Iasync		= 0x20,		/* intr. on async advance enable */
	Iall		= 0x3F,		/* all interrupts */

	/* Config reg. */
	Callmine	= 1,		/* route all ports to us */

	/* Portsc reg. */
	Pspresent	= 0x00000001,	/* device present */
	Psstatuschg	= 0x00000002,	/* Pspresent changed */
	Psenable	= 0x00000004,	/* device enabled */
	Pschange	= 0x00000008,	/* Psenable changed */
	Psresume	= 0x00000040,	/* resume detected */
	Pssuspend	= 0x00000080,	/* port suspended */
	Psreset		= 0x00000100,	/* port reset */
	Pspower		= 0x00001000,	/* port power on */
	Psowner		= 0x00002000,	/* port owned by companion */
	Pslinemask	= 0x00000C00,	/* line status bits */
	Pslow		= 0x00000400,	/* low speed device */

	/* Debug port csw reg. */
	Cowner	= 0x40000000,		/* port owned by ehci */
	Cenable	= 0x10000000,		/* debug port enabled */
	Cdone	= 0x00010000,		/* request is done */
	Cbusy	= 0x00000400,		/* port in use by a driver */
	Cerrmask= 0x00000380,		/* error code bits */
	Chwerr	= 0x00000100,		/* hardware error */
	Cterr	= 0x00000080,		/* transaction error */
	Cfailed	= 0x00000040,		/* transaction did fail */
	Cgo	= 0x00000020,		/* execute the transaction */
	Cwrite	= 0x00000010,		/* request is a write */
	Clen	= 0x0000000F,		/* data len */

	/* Debug port pid reg. */
	Prpidshift	= 16,		/* received pid */
	Prpidmask	= 0xFF,
	Pspidshift	= 8,		/* sent pid */
	Pspidmask	= 0xFF,
	Ptokshift	= 0,		/* token pid */
	Ptokmask	= 0xFF,

	Ptoggle		= 0x00008800,	/* to update toggles */
	Ptogglemask	= 0x0000FF00,

	/* Debug port addr reg. */
	Adevshift	= 8,		/* device address */
	Adevmask	= 0x7F,
	Aepshift	= 0,		/* endpoint number */
	Aepmask		= 0xF,
#endif
};

#ifdef WTF
/*
 * Capability registers (hw)
 */
struct Ecapio
{
	uint32_t	cap;		/* 00 controller capability register */
	uint32_t	parms;		/* 04 structural parameters register */
	uint32_t	capparms;	/* 08 capability parameters */
	uint32_t	portroute;	/* 0c not on the CS5536 */
};

/*
 * Debug port registers (hw)
 */
struct Edbgio
{
	uint32_t	csw;		/* control and status */
	uint32_t	pid;		/* USB pid */
	unsigned char	data[8];	/* data buffer */
	uint32_t	addr;		/* device and endpoint addresses */
};
#endif

struct Poll
{
	// There is already a Lock in Rendez.
	//Lock;
	Rendez;
	int	must;
	int	does;
};

struct Ctlr
{
	Rendez;                 /* for waiting to async advance doorbell */
	// There is already a Lock in Rendez.
	Lock lock;			/* for ilock. qh lists and basic ctlr I/O */
	QLock	portlck;	/* for port resets/enable... (and doorbell) */
	int	active;		/* in use or not */
	Pcidev*	pcidev;
	Ecapio*	capio;		/* Capability i/o regs */
	Eopio*	opio;		/* Operational i/o regs */

	int	nframes;	/* 1024, 512, or 256 frames in the list */
	uint32_t*	frames;		/* periodic frame list (hw) */
	Qh*	qhs;		/* async Qh circular list for bulk/ctl */
	Qtree*	tree;		/* tree of Qhs for the periodic list */
	int	ntree;		/* number of dummy qhs in tree */
	Qh*	intrqhs;		/* list of (not dummy) qhs in tree  */
	Isoio*	iso;		/* list of active Iso I/O */
	uint32_t	load;
	uint32_t	isoload;
	int	nintr;		/* number of interrupts attended */
	int	ntdintr;	/* number of intrs. with something to do */
	int	nqhintr;	/* number of async td intrs. */
	int	nisointr;	/* number of periodic td intrs. */
	int	nreqs;
	Poll	poll;
};

/*
 * PC-specific stuff
 */

/*
 * Operational registers (hw)
 */
struct Eopio
{
	uint32_t	cmd;		/* 00 command */
	uint32_t	sts;		/* 04 status */
	uint32_t	intr;		/* 08 interrupt enable */
	uint32_t	frno;		/* 0c frame index */
	uint32_t	seg;		/* 10 bits 63:32 of EHCI datastructs (unused) */
	uint32_t	frbase;		/* 14 frame list base addr, 4096-byte boundary */
	uint32_t	link;		/* 18 link for async list */
	unsigned char	d2c[0x40-0x1c];	/* 1c dummy */
	uint32_t	config;		/* 40 1: all ports default-routed to this HC */
	uint32_t	portsc[1];	/* 44 Port status and control, one per port */
};

extern int ehcidebug;
extern Ecapio *ehcidebugcapio;
extern int ehcidebugport;

void	ehcilinkage(Hci *hp);
void	ehcimeminit(Ctlr *ctlr);
void	ehcirun(Ctlr *ctlr, int on);
