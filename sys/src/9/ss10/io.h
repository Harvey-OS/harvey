/*
 * Device addresses
 */

/* bits [35:32] of phyiscal address */
#define	IOSPACE		0xF
#define	SBUSSPACE	0xE
#define	MEMSPACE	0x0

/* obio (on-board I/O?) devices. PA[32:35]=IOSPACE */
#define	ECC			0x00000000	/* ECC and VSIMM controls */
#define	IOMMU		0xE0000000	
#define	BUSCTL		0xE0001000	/* MBus and SBus controls */
#define	KMDUART		0xF1000000	/* keyboard A, mouse B */
#define	EIADUART	0xF1100000	/* serial ports */
#define	EEPROM		0xF1200000	/* NOT VERIFIED */
#define	NVRAM		0xF1201000	/* physical last page of NVRAM */
#define	BOOT_NVRAM	0xFFEDD000	/* boot ROM VIRTUAL last page of NVRAM */
#define	CLOCK		0xF1300000
#define	SYSCLOCK	0xF1310000
#define	CLOCKFREQ	1000000		/* one microsecond increments */
#define	INTRREG		0xF1400000	/* interrupt register */
#define	SYSINTRREG	0xF1410000	/* System interrupt register */
#define	FLOPPY		0xF1700000	/* AMD 82077 controller */
#define	AUXIO		0xF1800000	/* SPARC Auxilliary I/O */
#define	SYSCTLREG	0xF1F00000	/* System Control/Status Register */

/* sbus devices. PA[32:35]=SBUSSPACE */
#define	SBUSSLOT		0x10000000
#define	SBUS(n)		(0xE00000000+(n)*SBUSSLOT)
#define	FRAMEBUF(n)	SBUS(n)
#define	FRAMEBUFID(n)	(SBUS(n)+0x000000)
#define	DISPLAYRAM(n)	(SBUS(n)+0x800000)
#define	EPROM		0xF6000000
#define 	DMA			0xF0400000	/* DMA registers */
#define	SCSI			0xF0800000	/* NCR53C90 registers */
#define	ETHER		0xF0C00000	/* RDP, RAP, manual is wrong*/

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
#define	IDOFF	(4096-8-32)

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
#define RTCOFF		0xFF8
#define RTCREAD		(0x40)
#define RTCWRITE	(0x80)

/*
 * dma
 */
typedef struct DMAdev DMAdev;
struct DMAdev {
	/* ESP/SCSI DMA */
	ulong	csr;			/* Control/Status */
	ulong	addr;			/* address in 16Mb segment */
	ulong	count;			/* transfer byte count */
	ulong	diag;

	/* Ether DMA */
	ulong	ecsr;			/* Control/Status */
	ulong	ediag;
	ulong	cache;			/* cache valid bits */
	ulong	base;			/* base address (16Mb segment) */
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


/*
 * DMA2 ENET
 */
enum {
	E_Int_pend	= 0x00000001,	/* interrupt pending */
	E_Err_pend	= 0x00000002,	/* error pending */
	E_draining	= 0x0000000C,	/* E-cache draining */
	E_Int_en	= 0x00000010,	/* interrupt enable */
	E_Invalidate	= 0x00000020,	/* mark E-cache invalid */
	E_Slave_err	= 0x00000040,	/* slave access size error (sticky) */
	E_Reset		= 0x00000080,	/* invalidate cache & reset interface (sticky) */
	E_Drain		= 0x00000400,	/* force draining of E-cache to memory */
	E_Dsbl_wr_drn	= 0x00000800,	/* disable E-cache drain on descriptor writes from ENET */
	E_Dsbl_rd_drn	= 0x00001000,	/* disable E-cache drain on slave reads to ENET */
	E_Ilacc		= 0x00008000,	/* `modifies ENET DMA cycle' */
	E_Dsbl_buf_wr	= 0x00010000,	/* disable buffering of slave writes to ENET */
	E_Dsbl_wr_inval	= 0x00020000,	/* do not invalidate E-cache on slave writes */
	E_Burst_size	= 0x000C0000,	/* DMA burst size */
	E_Loop_test	= 0x00200000,	/* loop back mode */
	E_TP_select	= 0x00400000,	/* zero for AUI mode */
	E_Dev_id	= 0xF0000000,	/* device ID */
};
