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
/*	Purgefifo=	1<<31, /**/
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
typedef struct COLORdev COLORdev;
struct COLORdev {
	uchar	resv0[256];
	uchar	olaycol[16];	/* overlay colors (write 3 times: R G B) */
	uchar	pad0[112];
	uchar	curcol[3];	/* cursor colors (write 3 times: R G B) */
	uchar	pad1[125];
	uchar	id;		/* should be 0x4a (offset 0x200) */
	uchar	cmd0;		/* command register 0 */
	uchar	cmd1;		/* command register 1 */
	uchar	cmd2;		/* command register 2 */
	uchar	rmask;		/* pixel read mask */
	uchar	resv1;
	uchar	bmask;		/* pixel blink mask */
	uchar	resv2;
	uchar	ormask;		/* overlay read mask */
	uchar	obmask;		/* overlay blink mask */
	uchar	ileave;		/* interleave */
	uchar	test;		/* test register */
	uchar	rsig;		/* red signature */
	uchar	gsig;		/* green signature */
	uchar	bsig;		/* blue signature */
	uchar	pad2[241];
	uchar	curcmd;		/* cursor command (offset 0x300) */
	uchar	curxl;		/* cursor x (low) */
	uchar	curxh;		/* cursor x (high) */
	uchar	curyl;		/* cursor y (low) */
	uchar	curyh;		/* cursor y (high) */
	uchar	winxl;		/* window x (low) */
	uchar	winxh;		/* window x (high) */
	uchar	winyl;		/* window y (low) */
	uchar	winyh;		/* window y (high) */
	uchar	winwl;		/* window width (low) */
	uchar	winwh;		/* window width (high) */
	uchar	winhl;		/* window height (low) */
	uchar	winhh;		/* window height (high) */
	uchar	pad3[243];
	uchar	cursor[1024];	/* cursor RAM (64x64 (x 2 bits) ) (offset 0x400) */
};

#define COLORADDRL	IO(uchar, 0x4000003)
#define COLORADDRH	IO(uchar, 0x4080003)
#define COLOR		IO(COLORdev, 0x4100003)
#define COLORPALETTE	IO(uchar, 0x4180003)
#define COLORXREG	IO(uchar, 0x6000007)
#define COLORKREG	IO(uchar, 0x6080007)
#define COLORWMASKREG	IO(uchar, 0x6100000)
#define COLORFB		IOC(ulong, 0)
