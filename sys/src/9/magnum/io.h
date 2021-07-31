#define UNMAPPED	0x80000000
#define UNCACHED	0xA0000000
#define	IO(t,x)	((t *)(UNCACHED|0x10000000|(x)))
#define IOC(t,x) ((t *)(UNMAPPED|0x10000000|(x)))

/*
 *  duarts
 */
typedef struct SCCdev	SCCdev;
struct SCCdev
{
	uchar	dummy1[3];
	uchar	ptrb;
	uchar	dummy2[3];
	uchar	datab;
	uchar	dummy3[3];
	uchar	ptra;
	uchar	dummy4[3];
	uchar	dataa;
};
#define SCCADDR	IO(SCCdev, 0xB000000)

/*
 *  crystal frequency for SCC
 */
#define SCCFREQ	10000000

/*
 *  interrupt register
 */
enum {
	ISRce=		0x80,		/* modem call indicator */
	ISRdsr=		0x40,		/* data set ready */
	ISRdrs=		0x20,		/* data rate select */
	ISRlance=	0x10,		/* lance */
	ISRncr=		0x08,		/* ncr controller */
	ISRscc=		0x04,		/* serial controller */
	ISRkybd=	0x02,		/* keyboard */
	ISRslot=	0x01,		/* expansion slot */
};
#define ISR	IO(uchar, 0x9800003)

/*
 *  timer
 */
#define TCOUNT		IO(ulong, 0x1C000C00)
#define TBREAK		IO(ulong, 0x1C000D00)

/*
 *  lance
 */
#define LANCERAP	IO(ushort, 0x1A000006)
#define LANCERDP	IO(ushort, 0x1A000002)
#define CPU2LANCE	0	/* offset of lance memory in CPU space */
#define LANCESPAN	1	/* span (in shorts) between LANCE shorts in CPU space */

/*
 *  dma
 */
typedef struct DMAdev	DMAdev;
struct DMAdev
{
	ulong	laddr;		/* load address */
	uchar	pad1[252];
	ulong	hware;		/* ??? */
	uchar	pad2[254];
	ushort	fifo;		/* direct read/write to fifo */
	uchar	pad3[252];
	ulong	mode;		/* magic */
	uchar	pad4[254];
	ushort	block;		/* block count */
	uchar	pad5[252];
	ulong	caddr;		/* current address */
};
enum
{
	Purgefifo=	1<<31,
	Enable=		1<<30,
	Autoreload=	1<<29,
	Enableint=	1<<28,
	Tomemory=	1<<27,
	Clearerror=	1<<26,
	Fifofull=	1<<11,
	Empty=		1<<10,
	DMAerror=	1<<9,
	DMApend=	1<<8,
};
#define DMA1	IO(DMAdev, 0x1C000000)
#define DMA2	IO(DMAdev, 0x1C000600)

/*
 *  keyboard
 */
typedef struct KBDdev	KBDdev;
struct KBDdev
{
	uchar	pad1[3];
	uchar	data;		/* data and command operands */
	uchar	pad2[3];
	uchar	ctl;		/* command/status */
};
#define KBD	IO(KBDdev, 0x19000000)

/*
 *  random
 */
#define EREG	IO(ulong, 0x1C000E00)
#define CREG	IO(ulong, 0x1C000F00)
enum
{
	Buzz1542=	0<<4,	/* buzzer frequencies */
	Buzz762=	1<<4,
	Buzz381=	2<<4,
	Buzz190=	3<<4,
	Buzzdis=	1<<3,	/* turn off buzzer */
	Buzzbits=	7<<3,
	Penable=	1<<2,	/* enable parity checking */
	ClrEReg=	1<<1,	/* reset error latch of EReg */
	ClrErrIntB=	1<<0,	/* clear error interrupt */
};

/*
 *  non-volatile ram (every 4th location)
 */
typedef struct Nvram	Nvram;
struct Nvram
{
	uchar	pad[3];
	uchar	val;
};
#define	NVRAM	IO(Nvram, 0x1D000000)
#define NVLEN	2040

/*
 * real-time clock (every 4th location)
 */
typedef struct RTCdev	RTCdev;
struct RTCdev
{
	uchar	pad1[3];
	uchar	control;		/* read or write the device */
	uchar	pad2[3];
	uchar	sec;
	uchar	pad3[3];
	uchar	min;
	uchar	pad4[3];
	uchar	hour;
	uchar	pad5[3];
	uchar	wday;
	uchar	pad6[3];
	uchar	mday;
	uchar	pad7[3];
	uchar	mon;
	uchar	pad8[3];
	uchar	year;
};
#define RTC		IO(RTCdev, 0x1d001fe0)
#define RTCREAD		(0x40)
#define RTCWRITE	(0x80)

/*
 *  SCSI controller (every 4th location)
 */
typedef struct SCSIdev	SCSIdev;
struct SCSIdev
{
	uchar	pad0[3];
	uchar	countlo;	/* byte count, low bits */
	uchar	pad1[3];
	uchar	counthi;	/* byte count, hi bits */
	uchar	pad2[3];
	uchar	fifo;		/* data fifo */
	uchar	pad3[3];
	uchar	cmd;		/* command byte */
	union {
		struct {		/* read only... */
			uchar	pad04[3];
			uchar	status;		/* status */
			uchar	pad05[3];
			uchar	intr;		/* interrupt status */
			uchar	pad060[3];
			uchar	step;		/* sequence step */
			uchar	pad07[3];
			uchar	fflags;		/* fifo flags */
		};
		struct {		/* write only... */
			uchar	pad14[3];
			uchar	destid;		/* destination id */
			uchar	pad15[3];
			uchar	timeout;	/* during selection */
			uchar	pad16[3];
			uchar	syncperiod;	/* synchronous xfr period */
			uchar	pad17[3];
			uchar	syncoffset;	/* synchronous xfr offset */
		};
	};
	uchar	pad8[3];
	uchar	config;		/* configuration 1 */
	uchar	pad9[3];
	uchar	clock;		/* clock conversion factor (write only) */
	uchar	pada[3];
	uchar	test;		/* test mode (write only) */
	uchar	padb[3];
	uchar	config2;	/* configuration 2 */
	uchar	padc[3];
	uchar	config3;	/* configuration 3 */
	uchar	padd[3];
	uchar	resvd;		/* reserved */
	uchar	pade[3];
	uchar	resve;		/* reserved */
	uchar	padf[3];
	uchar	rfifo;		/* reserve fifo byte */
};
#define	SCSI	IO(SCSIdev, 0x18000000)

/*
 * Color graphics board (uses Brooktree 459 RAMDAC)
 */
#define COLORADDRL	IO(uchar, 0x4000003)
#define COLORADDRH	IO(uchar, 0x4080003)
#define COLOR		IO(uchar, 0x4100003)
#define COLORPALETTE	IO(uchar, 0x4180003)
#define COLORXREG	IO(uchar, 0x6000007)
#define COLORKREG	IO(uchar, 0x6080007)
#define COLORWMASKREG	IO(uchar, 0x6100000)
#define COLORFB		IOC(ulong, 0)

#define       PROM_RESET      0   /* run diags, check bootmode, reinit */
#define       PROM_EXEC       1   /* load new program image */
#define       PROM_RESTART    2   /* re-enter monitor command loop */
#define       PROM_REINIT     3   /* re-init monitor, then cmd loop */
#define       PROM_REBOOT     4   /* check bootmode, no config */
#define       PROM_AUTOBOOT   5   /* autoboot the system */

/*
 *  hs datakit board
 */
typedef struct HSdev	HSdev;
struct HSdev {
	ushort	version;
	ushort	pad0x001[3];
	ushort	csr;
	ushort	pad0x002[3];
	ushort	data;
	ushort	pad0x003[3];
	ushort	rearm;
};

/*
 *  incon datakit board
 */
typedef struct Incondev	Incondev;
struct Incondev {
	ushort ctl;
	ushort pad0x002[3];
	ushort csr;
	ushort pad0x010[11];
	ushort data;
};


/*
 *  hs device is at 0x200 in AT-space.
 *  the adapter requires it to be left-shifted by 2
 *  and the type of access to be coded in the bottom
 *  2 bits (0x2 means 16-bit access).
 *
 *  incon device is at 0x40 in AT-space.
 */
#define HSDEV		IO(HSdev, 0x802)
#define INCON		IO(Incondev, 0x102)
#define IMR		IO(ushort, 0x8020002)
#define INTCLEAR	IO(ulong, 0x400000)
