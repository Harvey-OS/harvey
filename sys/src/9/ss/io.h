#define	SBUS(n)		(0xF8000000+(n)*0x2000000)
#define	FRAMEBUF(n)	SBUS(n)
#define	FRAMEBUFID(n)	(SBUS(n)+0x000000)
#define	DISPLAYRAM(n)	(SBUS(n)+0x800000)
#define	EPROM		0xF6000000
#define	CLOCK		0xF3000000
#define	CLOCKFREQ	1000000		/* one microsecond increments */
#define	KMDUART		0xF0000000	/* keyboard A, mouse B */
#define	EIADUART	0xF1000000	/* serial ports */
#define	NVRAM		0xF2000000	/* non-volatile RAM */
#define DMA		0xF8400000	/* DMA registers */
#define SCSI		0xF8800000	/* NCR53C90 registers */
#define	ETHER		0xF8C00000	/* RDP, RAP */
#define	EEPROM		0xF2000000
#define	INTRREG		0xF5000000	/* interrupt register, IO space */

typedef struct SCCdev	SCCdev;
struct SCCdev
{
	uchar	ptrb;
	uchar	dummy1;
	uchar	datab;
	uchar	dummy2;
	uchar	ptra;
	uchar	dummy3;
	uchar	dataa;
	uchar	dummy4;
};

#define KMFREQ	10000000	/* crystal frequency for kbd/mouse 8530 */
#define EIAFREQ	5000000		/* crystal frequency for serial port 8530 */

/*
 *  non-volatile ram
 */
#define NVREAD	(2048-8)	/* minus RTC */
#define NVWRITE	(2048-8-32)	/* minus RTC, minus ID prom */

/*
 * real-time clock
 */
typedef struct RTCdev	RTCdev;
struct RTCdev
{
	uchar	control;		/* read or write the device */
	uchar	sec;
	uchar	min;
	uchar	hour;
	uchar	wday;
	uchar	mday;
	uchar	mon;
	uchar	year;
};
#define RTCOFF		0x7F8
#define RTCREAD		(0x40)
#define RTCWRITE	(0x80)

/*
 * dma
 */
typedef struct DMAdev DMAdev;
struct DMAdev {
	ulong	csr;			/* Control/Status */
	ulong	addr;			/* address in 16Mb segment */
	ulong	count;			/* transfer byte count */
	ulong	diag;
};

enum {
	Int_pend	= 0x00000001,	/* interrupt pending */
	Err_pend	= 0x00000002,	/* error pending */
	Pack_cnt	= 0x0000000C,	/* pack count (mask) */
	Int_en		= 0x00000010,	/* interrupt enable */
	Dma_Flush	= 0x00000020,	/* flush pack end error */
	Drain		= 0x00000040,	/* drain pack to memory */
	Dma_Reset	= 0x00000080,	/* hardware reset (sticky) */
	Write		= 0x00000100,	/* set for device to memory (!) */
	En_dma		= 0x00000200,	/* enable DMA */
	Req_pend	= 0x00000400,	/* request pending */
	Byte_addr	= 0x00001800,	/* next byte addr (mask) */
	En_cnt		= 0x00002000,	/* enable count */
	Tc		= 0x00004000,	/* terminal count */
	Ilacc		= 0x00008000,	/* which ether chip */
	Dev_id		= 0xF0000000,	/* device ID */
};

/*
 *  NCR53C90 SCSI controller (every 4th location)
 */
typedef struct SCSIdev	SCSIdev;
struct SCSIdev {
	uchar	countlo;		/* byte count, low bits */
	uchar	pad1[3];
	uchar	countmi;		/* byte count, middle bits */
	uchar	pad2[3];
	uchar	fifo;			/* data fifo */
	uchar	pad3[3];
	uchar	cmd;			/* command byte */
	uchar	pad4[3];
	union {
		struct {		/* read only... */
			uchar	status;		/* status */
			uchar	pad05[3];
			uchar	intr;		/* interrupt status */
			uchar	pad06[3];
			uchar	step;		/* sequence step */
			uchar	pad07[3];
			uchar	fflags;		/* fifo flags */
			uchar	pad08[3];
			uchar	config;		/* RW: configuration */
			uchar	pad09[3];
			uchar	Reserved1;
			uchar	pad0A[3];
			uchar	Reserved2;
			uchar	pad0B[3];
			uchar	conf2;		/* RW: configuration */
			uchar	pad0C[3];
			uchar	conf3;		/* RW: configuration */
			uchar	pad0D[3];
			uchar	partid;		/* unique part id */
			uchar	pad0E[3];
			uchar	fbottom;	/* RW: fifo bottom */
			uchar	pad0F[3];
		};
		struct {		/* write only... */
			uchar	destid;		/* destination id */
			uchar	pad15[3];
			uchar	timeout;	/* during selection */
			uchar	pad16[3];
			uchar	syncperiod;	/* synchronous xfr period */
			uchar	pad17[3];
			uchar	syncoffset;	/* synchronous xfr offset */
			uchar	pad18[3];
			uchar	RW0;
			uchar	pad19[3];
			uchar	clkconf;
			uchar	pad1A[3];
			uchar	test;	
			uchar	pad1B[3];
			uchar	RW1;
			uchar	pad1C[3];
			uchar	RW2;
			uchar	pad1D[3];
			uchar	counthi;	/* byte count, hi bits */
			uchar	pad1E[3];
			uchar	RW3;
			uchar	pad1F[3];
		};
	};
};
