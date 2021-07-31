#define	IOACK(t, x)	((t *)(IOBASE+(x)))
#define	IONAK(t, x)	((t *)(IOBASE+0x08000000+(x)))

#define SCSIENABLE	0x00000001
#define IOENABLE	0x00000004

#define IOCTLADDR	IONAK(ulong, 0)

/*
 * Interrupt control
 */
typedef struct {
	ulong	mask0;
	ulong	mask1;
	ulong	status0;
	ulong	status1;
} Irq;
#define IRQADDR		IONAK(Irq, 0x0080)

#define IRQQUART	1		/* 0x0002 */
#define IRQINCON	5		/* 0x0020 */
#define IRQSCSIDMA	8		/* 0x0100 */
#define IRQSONIC	9		/* 0x0200 */
#define IRQSCSI		10		/* 0x0400 */
#define NIRQ		16

/*
 * Signetics 68C94
 * Quad Universal Asynchronous Receiver/Transmitter
 * (UARTS are the Devil's Children)
 */
typedef	struct {
	unsigned mr;			/* 0x00:AB:RW: Mode Channels 0, 1 & 2 */
	union {
		unsigned sr;		/* 0x01:AB:R : Status */
		unsigned csr;		/* 0x01:AB: W: Clock Select */
	};
	unsigned cr;			/* 0x02:AB: W: Command */
	union {
		unsigned rxfifo;	/* 0x03:AB:R : Receive Holding */
		unsigned txfifo;	/* 0x03:AB: W: Transmit Holding */
	};
} Uart;

typedef	struct {
	Uart	a;

	union {
		unsigned ipcr;		/* 0x04:A :R : Input Port Change */
		unsigned acr;		/* 0x04:A : W: Auxiliary Control */
	};
	union {
		unsigned isr;		/* 0x05:A :R : Interrupt Status */
		unsigned imr;		/* 0x05:A : W: Interrupt Mask */
	};
	unsigned ctu;			/* 0x06:A :RW: Counter/Timer Upper */
	unsigned ctl;			/* 0x07:A :RW: Counter/Timer Lower */

	Uart	b;

	unsigned opr;			/* 0x0C: B:RW: Output Port */
	union {
		unsigned ipr;		/* 0x0D: B:R : Input Port */
		unsigned iopcra;	/* 0x0D: B: W: I/O Port Config for a */
	};
	union {
		unsigned start;		/* 0x0E: B:R : Start Counter */
		unsigned iopcrb;	/* 0x0E: B: W: I/O Port Config for b */
	};
	unsigned stop;			/* 0x0F: B:R : Stop Counter */
} Duart;

typedef struct {
	Duart	duart[2];

	unsigned bcr[4];		/* 0x20:RW: Bidding control */
	unsigned pwrdn;			/* 0x24: W: Power-Down */
	unsigned pwrup;			/* 0x25: W: Power-Up */
	unsigned nak;			/* 0x26: W: Disable DACKN */
	unsigned ack;			/* 0x27: W: Enable DACKN */
	unsigned cir;			/* 0x28:R : Current Interrupt */
	unsigned gicr;			/* 0x29:R : Global Interupt Channel */
	union {
		unsigned gibcr;		/* 0x2A:R : Global Interrupt byte count */
		unsigned update;	/* 0x2A: W: CIR Update */
	};
	union {
		unsigned grxfifo;	/* 0x2B:R : Global Receive Holding */
		unsigned gtxfifo;	/* 0x2B: W: Global Transmit Holding */
	};
	unsigned icr;			/* 0x2C:RW: Interrupt Control */
	unsigned brg;			/* 0x2D: W: BRG Rate */
	unsigned setclk[2];		/* 0x2E-2F: Set X1/CLK */
	unsigned rsv30[16];		/* 0x30-3F: reserved */
} Quart;
#define EIAADDR		IOACK(Quart, 0x0100)

#ifdef notdef
typedef struct {
	ulong	ctl;
	ulong	csr;
	ulong	pad0x004[2];
	ulong	data;
} Incon;
#define INCONADDR	IOACK(Incon, 0x0000)
#endif

/*
 * MK48T02 Timekeeper RAM
 */
typedef struct {
	uchar	val;
	uchar	pad[3];
} Nvram;
#define NVRAMADDR	IONAK(Nvram, 0x2000)
#define NVRAMLEN	2040

typedef struct RTCdev {
	ulong	control;		/* read or write the device */
	ulong	sec;
	ulong	min;
	ulong	hour;
	ulong	wday;
	ulong	mday;
	ulong	mon;
	ulong	year;
} RTCdev;
#define RTCADDR		IONAK(RTCdev, 0x3FE0)

/*
 * PCF8584 IIC-bus Controller
 */
typedef struct {
	ulong	data;
	ulong	ctl;
} Iic;
#define IICADDR		IOACK(Iic, 0x0080)

/*
 * Structure of the data held in the PCF8582
 * EEPROM on the IIC-bus.
 */
typedef struct Eeprom {
	uchar	magic[5];
	uchar	ea[6];
	uchar	mask;
	uchar	clock;
	uchar	dsp;
	uchar	pcb;
	uchar	rev;
} Eeprom;
#define EEPROMMAGIC	"A600D92010"

/*
 * WD33C93A SCSI Bus Interface Controller
 */
typedef struct {
	ulong	addr;
	ulong	data;
} SCSIdev;
#define SCSIADDR	IONAK(ulong, 0x0100)
#define SCSIDATA	IONAK(ulong, 0x0104)
#define SCSIPDMA	IONAK(ulong, 0x0180)

/*
 * National Semiconductor DP83932
 * Systems-Oriented Network Interface Controller
 * (SONIC)
 */
typedef struct {
	ulong	cr;		/* command */
	ulong	dcr;		/* data configuration */
	ulong	rcr;		/* receive control */
	ulong	tcr;		/* transmit control */
	ulong	imr;		/* interrupt mask */
	ulong	isr;		/* interrupt status */
	ulong	utda;		/* upper transmit descriptor address */
	ulong	ctda;		/* current transmit descriptor address */
	ulong	pad0x08[5];	/*  */
	ulong	urda;		/* upper receive descriptor address */
	ulong	crda;		/* current receive descriptor address */
	ulong	pad0x0F[4];	/*  */
	ulong	eobc;		/* end of buffer word count */
	ulong	urra;		/* upper receive resource address */
	ulong	rsa;		/* resource start address */
	ulong	rea;		/* resource end address */
	ulong	rrp;		/* resource read pointer */
	ulong	rwp;		/* resource write pointer */
	ulong	pad0x19[8];	/*  */
	ulong	cep;		/* CAM entry pointer */
	ulong	cap2;		/* CAM address port 2 */
	ulong	cap1;		/* CAM address port 1 */
	ulong	cap0;		/* CAM address port 0 */
	ulong	ce;		/* CAM enable */
	ulong	cdp;		/* CAM descriptor pointer */
	ulong	cdc;		/* CAM descriptor count */
	ulong	sr;		/* silicon revision */
	ulong	wt0;		/* watchdog timer 0 */
	ulong	wt1;		/* watchdog timer 1 */
	ulong	rsc;		/* receive sequence counter */
	ulong	crct;		/* CRC error tally */
	ulong	faet;		/* FAE tally */
	ulong	mpt;		/* missed packet tally */
	ulong	mdt;		/* maximum deferral timer */
	ulong	pad0x30[15];	/*  */
	ulong	dcr2;		/* data configuration 2 */
} Sonic;
#define SONICADDR	((Sonic*)0xA4400000)

/*
 * Bart's LCD
 */
typedef struct {
	ulong	ctl;
	ulong	data;
} Lcdisp;
#define LCDADDR		IOACK(Lcdisp, 0x3000)
