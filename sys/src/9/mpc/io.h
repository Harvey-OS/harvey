typedef struct BD BD;
typedef struct Ring Ring;
typedef struct GTimer GTimer;

enum
{
	/* interrupt vectors (SIU and CPM) */
	VectorPIC= 0,	/* level 0 to level 7, assigned by software */
		/* vector assignments are determined by the assignments here */
		PITlevel=	2,
		CPIClevel=	4,
		PCMCIAlevel=	6,
		RTClevel=	7,
	VectorIRQ=	VectorPIC+8,	/* IRQ0 to IRQ7 */
	VectorCPIC=	VectorIRQ+8,	/* 32 CPM interrupts: 0 (error) to 0x1F (PC15) */
};

/*
 * these are defined to keep the interface compatible with other
 * architectures, but only BUSUNKNOWN is currently used
 */
#define MKBUS(t,b,d,f)	(((t)<<24)|(((b)&0xFF)<<16)|(((d)&0x1F)<<11)|(((f)&0x07)<<8))
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)
#define BUSUNKNOWN	(-1)

/*
 * Buffer Descriptors and IO Rings
 */

struct BD {
	ushort	status;
	ushort	length;
	ulong	addr;
};

BD*	bdalloc(int);
void	bdfree(BD*, int);

enum {
	/* Rx BDs, bits common to all protocols */
	BDEmpty=	1<<15,
	BDWrap=		1<<13,
	BDInt=		1<<12,
	BDLast=		1<<11,
	BDFirst=		1<<10,

	/* Tx BDs */
	BDReady=		1<<15,
	/* BDWrap, BDInt, BDLast */
};


struct Ring {
	BD*	rdr;				/* receive descriptor ring */
	void*	rrb;				/* receive ring buffers */
	int	rdrx;				/* index into rdr */
	int	nrdre;			/* length of rdr */

	BD*	tdr;				/* transmit descriptor ring */
	Block**	txb;				/* corresponding transmit ring buffers */
	int	tdrh;				/* host index into tdr */
	int	tdri;				/* interface index into tdr */
	int	ntdre;			/* length of tdr */
	int	ntq;				/* pending transmit requests */
};

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

int	ioringinit(Ring*, int, int, int);

/*
 * CPM
 */
enum {
	/* commands */
	InitRxTx =	0,
	InitRx =		1,
	InitTx =		2,
	EnterHunt=	3,
	StopTx=		4,
	GracefulStopTx = 5,
	InitIDMA =	5,
	RestartTx =	6,
	CloseRxBD =	7,
	SetGroupAddr = 8,
	SetTimer =	8,
	GCITimeout =	9,
	GCIAbort =	10,
	StopIDMA =	11,
	StartDSP = 	12,
	ArmIDMA =	13,
	InitDSP =		13,
	USBCmd =	15,

	/* channel IDs */
	SCC1ID=	0,
	USBID=	0,
	I2CID=	1,
	IDMA1ID= 1,
	SCC2ID=	4,
	SPIID=	5,
	IDMA2ID= 5,
	TIMERID=	5,
	SCC3ID=	8,
	SMC1ID=	9,
	DSP1ID=9,
	SCC4ID=	12,
	SMC2ID=	13,
	DSP2ID=	13,

	/* bgcr */
	BaudEnable = 1<<16,

	/* sicr */
	CLK1 = 4,		/* SCC1,2 */
	CLK2 = 5,
	CLK3 = 6,
	CLK4 = 7,
	CLK5 = CLK1,	/* SCC3,4 */
	CLK6 = CLK2,
	CLK7 = CLK3,
	CLK8 = CLK4,
};

void	cpmop(int, int, int);
void*	cpmalloc(int, int);
void	cpmfree(void*, int);
void*	cpmparam(ulong, int);
IMM*	ioplock(void);
void	iopunlock(void);
void	sccnmsi(int, int, int);
int	scc2claim(int, ulong, void(*)(void*), void*);
void	scc2stop(void*);

/*
 * CPM timers
 */
enum {
	/* timer modes */
	CaptureRise=	1<<6,
	CaptureFall=	2<<6,
	CaptureEdge=	3<<6,
	TimerToggle=	1<<5,	/* toggle TOUTx* pin */
	TimerORI=	1<<4,	/* Output Reference Interrupt */
	TimerRestart=	1<<3,
	TimerSclk=	1<<1,
	TimerSclk16=	2<<1,
	TimerTIN=	3<<1,	/* clock by falling edge of TINx */
	TimerGate=	1<<0,	/* TGATE1* controls timer */

	/* timer events */
	TimerREF=	1<<1,
	TimerCAP=	1<<0
};

struct GTimer{
	int	x;
	int	inuse;
	int	event;
	ushort*	tmr;
	ushort*	trr;
	ushort*	tcr;
	ushort*	tcn;
	ushort*	ter;
	void*	arg;
	void	(*interrupt)(Ureg*, void*, GTimer*);
};
GTimer*	gtimer(ushort, ushort, void (*)(Ureg*,void*,GTimer*), void*);
void	gtimerset(GTimer*, ushort, int);
void	gtimerstart(GTimer*);
void	gtimerstop(GTimer*);
void	gtimerfree(GTimer*);

enum {
	/* BCSR1 bits */
	DisableFlash=	IBIT(0),
	DisableDRAM=	IBIT(1),
	DisableEther=	IBIT(2),
	DisableIR=	IBIT(3),
	DisableRS232a=	IBIT(7),
	DisablePCMCIA=	IBIT(8),
	PCCVCCMask=	IBIT(9)|IBIT(15),
	PCCVPPMask=	IBIT(10)|IBIT(11),
	DisableRS232b=	IBIT(13),
	EnableSDRAM=	IBIT(14),

	PCCVCC0V=	IBIT(15)|IBIT(9),
	PCCVCC5V=	IBIT(9),	/* active low */
	PCCVCC3V=	IBIT(15),	/* active low */
	PCCVPP0V=	IBIT(10)|IBIT(11),	/* active low */
	PCCVPP5V=	IBIT(10),	/* active low */
	PCCVPP12V=	IBIT(11),	/* active low */
	PCCVPPHiZ=	IBIT(10)|IBIT(11),

	/* BCSR4 bits */
	DisableTPDuplex=	IBIT(1),
	DisableLamp=	IBIT(3),
	DisableUSB=	IBIT(4),
	USBFullSpeed=	IBIT(5),
	DisableUSBVcc=	IBIT(6),
	DisableVideoLamp=	IBIT(8),
	EnableVideoClock=	IBIT(9),
	EnableVideoPort=	IBIT(10),
	DisableModem=	IBIT(11),
};

/*
 * everything in this file follows a hardware/firmware layout in the 8xx manuals:
 * mind the data types, offsets and alignment
 */

/*
 * basic IO controller parameters (SMC and SCC)
 */
typedef struct IOCparam IOCparam;
struct IOCparam {
	ushort	rbase;
	ushort	tbase;
	uchar	rfcr;
	uchar	tfcr;
	ushort	mrblr;
	ulong	rstate;
	ulong	rxidp;
	ushort	rbptr;
	ushort	rxibc;
	ulong	rxtemp;
	ulong	tstate;
	ulong	txidp;
	ushort	tbptr;
	ushort	txibc;
	ulong	txtemp;
};

typedef struct SCCparam SCCparam;
struct SCCparam {
	IOCparam;
	ulong	rcrc;
	ulong	tcrc;
};

typedef struct SCC SCC;
struct SCC {
	ulong	gsmrl;
	ulong	gsmrh;
	ushort	psmr;
	uchar	rsvscc0[2];
	ushort	todr;
	ushort	dsr;
	ushort	scce;
	uchar	rsvscc1[2];
	ushort	sccm;
	uchar	rsvscc3;
	uchar	sccs;
	ushort	irmode;
	ushort	irsip;
};

typedef struct SMC SMC;
struct SMC {
	uchar	pad1[2];
	ushort	smcmr;
	uchar	pad2[2];
	uchar	smce;
	uchar	pad3[3];
	uchar	smcm;
	uchar	pad4[5];
};

typedef struct SPI SPI;
struct SPI {
	ushort	spmode;
	uchar	res1[4];
	uchar	spie;
	uchar	res2[3];
	uchar	spim;
	uchar	res3[2];
	uchar	spcom;
	uchar	res4[10];
};

typedef struct USB USB;
struct USB {
	uchar	usmod;
	uchar	usadr;
	uchar	uscom;
	uchar	rsvu1;
	ushort	usep[4];
	uchar	rsvu2[4];
	ushort	usber;
	uchar	rsvu3[2];
	ushort	usbmr;
	uchar	rsvu4;
	uchar	usbs;
	uchar	rsvu5[8];
};

typedef struct IMM IMM;
struct IMM {
	struct {	/* general SIU */
		ulong	siumcr;
		ulong	sypcr;
		uchar	rsv0[0xE-0x8];
		ushort	swsr;
		ulong	sipend;
		ulong	simask;
		ulong	siel;
		uchar	sivec;
		uchar	padv[3];
		ulong	tesr;
		uchar	rsv1[0x30-0x24];
		ulong	sdcr;
		uchar	rsv2[0x80-0x34];
	};
	struct {	/* PCMCIA */
		struct {
			ulong	base;
			ulong	option;
		} pcmr[8];
		uchar	rsv3[0xe0-0xc0];
		ulong	pgcr[2];
		ulong	pscr;
		uchar	rsv4[0xf0-0xec];
		ulong	pipr;
		uchar	rsv5[4];
		ulong	per;
		uchar	rsv6[4];
	};
	struct {	/* MEMC */
		struct {
			ulong	base;
			ulong	option;
		} memc[8];
		uchar	rsv7a[0x24];
		ulong	mar;
		ulong	mcr;
		uchar	rsv7b[4];
		ulong	mamr;
		ulong	mbmr;
		ushort	mstat;
		ushort	mptpr;
		ulong	mdr;
		uchar	rsv7c[0x80];
	};
	struct {	/* system integration timers */
		ushort	tbscr;
		uchar	rsv8a[2];
		ulong	tbrefu;
		ulong	tbrefl;
		uchar	rsv8b[0x14];
		ushort	rtcsc;
		uchar	rsv8c[2];
		ulong	rtc;
		ulong	rtsec;
		ulong	rtcal;
		uchar	rsv8d[0x10];
		ushort	piscr;
		ushort	rsv8e;
		ulong	pitc;
		ulong	pitr;
		uchar	rsv8f[0x34];
	};
	struct {	/* 280: clocks and resets */
		ulong	sccr;
		ulong	plprcr;
		ulong	rsr;
		uchar	rsv9[0x300-0x28c];
	};
	struct {	/* 300: system integration timers keys */
		ulong	tbscrk;
		ulong	tbrefuk;
		ulong	tbreflk;
		ulong	tbk;
		uchar	rsv10a[0x10];
		ulong	rtcsck;
		ulong	rtck;
		ulong	rtseck;
		ulong	rtcalk;
		uchar	rsv10b[0x10];
		ulong	piscrk;
		ulong	pitck;
		uchar	rsv10c[0x38];
	};
	struct {	/* 380: clocks and resets keys */
		ulong	sccrk;
		ulong	plprcrk;
		ulong	rsrk;
		uchar	rsv11[0x800-0x38C];
	};
	struct {	/* 800: video controller */
		ushort	vccr;
		ushort	pad11a;
		uchar	vsr;
		uchar	pad11b;
		uchar	vcmr;
		uchar	pad11c;
		ulong	vbcb;
		ulong	pad11d;
		ulong	vfcr0;
		ulong	vfaa0;
		ulong	vfba0;
		ulong	vfcr1;
		ulong	vfaa1;
		ulong	vfba1;
		uchar	rsv11a[0x840-0x828];
	};
	struct {	/* 840: LCD */
		ulong	lccr;
		ulong	lchcr;
		ulong	lcvcr;
		ulong	rsv11b;
		ulong	lcfaa;
		ulong	lcfba;
		uchar	lcsr;
		uchar	rsv11c[0x860-0x859];
	};
	struct {	/* 860: I2C */
		uchar	i2mod;
		uchar	rsv12a[3];
		uchar	i2add;
		uchar	rsv12b[3];
		uchar	i2brg;
		uchar	rsv12c[3];
		uchar	i2com;
		uchar	rsv12d[3];
		uchar	i2cer;
		uchar	rsv12e[3];
		uchar	i2cmr;
		uchar	rsv12[0x900-0x875];
	};
	struct {	/* 900: DMA */
		uchar	rsv13[4];
		ulong	sdar;
		uchar	sdsr;
		uchar	pad1[3];
		uchar	sdmr;
		uchar	pad2[3];
		uchar	idsr1;
		uchar	pad3[3];
		uchar	idmr1;
		uchar	pad4[3];
		uchar	idsr2;
		uchar	pad5[3];
		uchar	idmr2;
		uchar	pad6[0x930-0x91D];
	};
	struct {	/* CPM interrupt control */
		ushort	civr;
		uchar	pad7[0x940-0x932];
		ulong	cicr;
		ulong	cipr;
		ulong	cimr;
		ulong	cisr;
	};
	struct {	/* input/output port */
		ushort	padir;
		ushort	papar;
		ushort	paodr;
		ushort	padat;
		uchar	pad8[8];
		ushort	pcdir;
		ushort	pcpar;
		ushort	pcso;
		ushort	pcdat;
		ushort	pcint;
		uchar	pad9[6];
		ushort	pddir;
		ushort	pdpar;
		ushort	rsv14a;
		ushort	pddat;
		uchar	rsv14[0x980-0x978];
	};
	struct {	/* CPM timers */
		ushort	tgcr;
		uchar	rsv15a[0x990-0x982];
		ushort	tmr1;
		ushort	tmr2;
		ushort	trr1;
		ushort	trr2;
		ushort	tcr1;
		ushort	tcr2;
		ushort	tcn1;
		ushort	tcn2;
		ushort	tmr3;
		ushort	tmr4;
		ushort	trr3;
		ushort	trr4;
		ushort	tcr3;
		ushort	tcr4;
		ushort	tcn3;
		ushort	tcn4;
		ushort	ter1;
		ushort	ter2;
		ushort	ter3;
		ushort	ter4;
		uchar	rsv15[0x9C0-0x9B8];
	};
	struct {	/* CPM */
		ushort	cpcr;
		uchar	res0[2];
		ushort	rccr;
		uchar	res1;
		uchar	rmds;
		uchar	res2a[4];
		ushort	rctr1;
		ushort	rctr2;
		ushort	rctr3;
		ushort	rctr4;
		uchar	res2[2];
		ushort	rter;
		uchar	res3[2];
		ushort	rtmr;
		uchar	rsv16[0x9F0-0x9DC];
	};
	union {	/* BRG */
		struct {
			ulong	brgc1;
			ulong	brgc2;
			ulong	brgc3;
			ulong	brgc4;
		};
		ulong	brgc[4];
	};
	uchar	skip0[0xAB2-0xA00];	/* USB, SCC, SMC, SPI: address using IOREGS in fns.h */
	struct {	/* PIP */
		ushort	pipc;		/* not 823 */
		ushort	ptpr;		/* not 823 */
		ulong	pbdir;
		ulong	pbpar;
		uchar	pad10[2];
		ushort	pbodr;
		ulong	pbdat;
		uchar	pad11[0xAE0-0xAC8];
	};
	struct {	/* SI */
		ulong	simode;
		uchar	sigmr;
		uchar	pad12;
		uchar	sistr;
		uchar	sicmr;
		uchar	pad13[4];
		ulong	sicr;
		ulong	sirp;
		uchar	pad14[0xB00-0xAF4];
	};
	uchar	vcram[256];
	uchar	siram[512];
	ushort	lcdmap[256];
};


enum {
	BusCBUS		= 0,		/* Corollary CBUS */
	BusCBUSII,			/* Corollary CBUS II */
	BusEISA,			/* Extended ISA */
	BusFUTURE,			/* IEEE Futurebus */
	BusINTERN,			/* Internal bus */
	BusISA,				/* Industry Standard Architecture */
	BusMBI,				/* Multibus I */
	BusMBII,			/* Multibus II */
	BusMCA,				/* Micro Channel Architecture */
	BusMPI,				/* MPI */
	BusMPSA,			/* MPSA */
	BusNUBUS,			/* Apple Macintosh NuBus */
	BusPCI,				/* Peripheral Component Interconnect */
	BusPCMCIA,			/* PC Memory Card International Association */
	BusTC,				/* DEC TurboChannel */
	BusVL,				/* VESA Local bus */
	BusVME,				/* VMEbus */
	BusXPRESS,			/* Express System Bus */
};

/*
 * PCMCIA support code.
 */
/*
 * Map between ISA memory space and PCMCIA card memory space.
 */
struct PCMmap {
	ulong	ca;			/* card address */
	ulong	cea;			/* card end address */
	ulong	isa;			/* ISA address */
	int	len;			/* length of the ISA area */
	int	attr;			/* attribute memory */
	int	ref;
};
