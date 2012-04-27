/*
 * ECHI portable hardware definitions
 */

typedef struct Ecapio Ecapio;
typedef struct Edbgio Edbgio;

#pragma incomplete Ecapio;
#pragma incomplete Edbgio;

/*
 * EHCI interface registers and bits
 */
enum
{
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
	/* interrupt threshold ctl. in µframes (1-32 in powers of 2) */
	Citcshift	= 16,
	Citcmask	= 0xff << Citcshift,

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
 * Debug port registers (hw)
 */
struct Edbgio
{
	ulong	csw;		/* control and status */
	ulong	pid;		/* USB pid */
	uchar	data[8];	/* data buffer */
	ulong	addr;		/* device and endpoint addresses */
};
