typedef struct Ecapio Ecapio;
typedef struct Eopio Eopio;
typedef struct Edbgio Edbgio;

/*
 * EHCI interface registers and bits
 */
enum
{
	Cnports		= 0xF,		/* nport bits in Ecapio parms. */
	Cdbgportshift	= 20,		/* debug port in Ecapio parms. */
	Cdbgportmask	= 0xF,
	C64		= 1,		/* 64-bits, in Ecapio capparms. */
	Ceecpshift	= 8,		/* extended capabilities ptr. in */
	Ceecpmask	= 8,		/* the Ecapio capparms reg. */
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
	Citc1		= 0x10000,	/* interrupt threshold ctl. 1 µframe */
	Citc4		= 0x40000,	/* same. 2 µframes */
	/* ... */
	Citc8		= 0x80000,	/* same. 8 µframes (can go up to 64) */

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
	Callmine		= 1,		/* route all ports to us */

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
	Aepshift		= 0,		/* endpoint number */
	Aepmask		= 0xF,
};

/*
 * Capability registers (hw)
 */
struct Ecapio
{
	ulong	cap;		/* 00 controller capability register */
	ulong	parms;		/* 04 structural parameters register */
	ulong	capparms;	/* 08 capability parameters */
	ulong	portroute;	/* 0c not on the CS5536 */
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

/*
 * Debug port registers (hw)
 */
struct Edbgio
{
	ulong	csw;		/* control and status */
	ulong	pid;		/* USB pid */
	uchar	data[8];	/* data buffer */
	ulong	addr;		/* device and endpoint addresses */
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
