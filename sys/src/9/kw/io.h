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
	BUSUNKNOWN = -1
};

#define MKBUS(t,b,d,f)	(((t)<<24)|(((b)&0xFF)<<16)|(((d)&0x1F)<<11)|(((f)&0x07)<<8))
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)

/*
 * PCI support code.
 */
enum {					/* type 0 & type 1 pre-defined header */
	PciVID		= 0x00,		/* vendor ID */
	PciDID		= 0x02,		/* device ID */
	PciPCR		= 0x04,		/* command */
	PciPSR		= 0x06,		/* status */
	PciRID		= 0x08,		/* revision ID */
	PciCCRp		= 0x09,		/* programming interface class code */
	PciCCRu		= 0x0A,		/* sub-class code */
	PciCCRb		= 0x0B,		/* base class code */
	PciCLS		= 0x0C,		/* cache line size */
	PciLTR		= 0x0D,		/* latency timer */
	PciHDT		= 0x0E,		/* header type */
	PciBST		= 0x0F,		/* BIST */
};

/* ccrb (base class code) values; controller types */
enum {
	Pcibcpci1	= 0,		/* pci 1.0; no class codes defined */
	Pcibcstore	= 1,		/* mass storage */
	Pcibcnet	= 2,		/* network */
	Pcibcdisp	= 3,		/* display */
	Pcibcmmedia	= 4,		/* multimedia */
	Pcibcmem	= 5,		/* memory */
	Pcibcbridge	= 6,		/* bridge */
	Pcibccomm	= 7,		/* simple comms (e.g., serial) */
	Pcibcbasesys	= 8,		/* base system */
	Pcibcinput	= 9,		/* input */
	Pcibcdock	= 0xa,		/* docking stations */
	Pcibcproc	= 0xb,		/* processors */
	Pcibcserial	= 0xc,		/* serial bus (e.g., USB) */
	Pcibcwireless	= 0xd,		/* wireless */
	Pcibcintell	= 0xe,		/* intelligent i/o */
	Pcibcsatcom	= 0xf,		/* satellite comms */
	Pcibccrypto	= 0x10,		/* encryption/decryption */
	Pcibcdacq	= 0x11,		/* data acquisition & signal proc. */
};

/* ccru (sub-class code) values; common cases only */
enum {
	/* mass storage */
	Pciscscsi	= 0,		/* SCSI */
	Pciscide	= 1,		/* IDE (ATA) */

	/* network */
	Pciscether	= 0,		/* Ethernet */

	/* display */
	Pciscvga	= 0,		/* VGA */
	Pciscxga	= 1,		/* XGA */
	Pcisc3d		= 2,		/* 3D */

	/* bridges */
	Pcischostpci	= 0,		/* host/pci */
	Pciscpcicpci	= 1,		/* pci/pci */

	/* simple comms */
	Pciscserial	= 0,		/* 16450, etc. */
	Pciscmultiser	= 1,		/* multiport serial */

	/* serial bus */
	Pciscusb	= 3,		/* USB */
};

enum {					/* type 0 pre-defined header */
	PciCIS		= 0x28,		/* cardbus CIS pointer */
	PciSVID		= 0x2C,		/* subsystem vendor ID */
	PciSID		= 0x2E,		/* cardbus CIS pointer */
	PciEBAR0	= 0x30,		/* expansion ROM base address */
	PciMGNT		= 0x3E,		/* burst period length */
	PciMLT		= 0x3F,		/* maximum latency between bursts */
};

enum {					/* type 1 pre-defined header */
	PciPBN		= 0x18,		/* primary bus number */
	PciSBN		= 0x19,		/* secondary bus number */
	PciUBN		= 0x1A,		/* subordinate bus number */
	PciSLTR		= 0x1B,		/* secondary latency timer */
	PciIBR		= 0x1C,		/* I/O base */
	PciILR		= 0x1D,		/* I/O limit */
	PciSPSR		= 0x1E,		/* secondary status */
	PciMBR		= 0x20,		/* memory base */
	PciMLR		= 0x22,		/* memory limit */
	PciPMBR		= 0x24,		/* prefetchable memory base */
	PciPMLR		= 0x26,		/* prefetchable memory limit */
	PciPUBR		= 0x28,		/* prefetchable base upper 32 bits */
	PciPULR		= 0x2C,		/* prefetchable limit upper 32 bits */
	PciIUBR		= 0x30,		/* I/O base upper 16 bits */
	PciIULR		= 0x32,		/* I/O limit upper 16 bits */
	PciEBAR1	= 0x28,		/* expansion ROM base address */
	PciBCR		= 0x3E,		/* bridge control register */
};

enum {					/* type 2 pre-defined header */
	PciCBExCA	= 0x10,
	PciCBSPSR	= 0x16,
	PciCBPBN	= 0x18,		/* primary bus number */
	PciCBSBN	= 0x19,		/* secondary bus number */
	PciCBUBN	= 0x1A,		/* subordinate bus number */
	PciCBSLTR	= 0x1B,		/* secondary latency timer */
	PciCBMBR0	= 0x1C,
	PciCBMLR0	= 0x20,
	PciCBMBR1	= 0x24,
	PciCBMLR1	= 0x28,
	PciCBIBR0	= 0x2C,		/* I/O base */
	PciCBILR0	= 0x30,		/* I/O limit */
	PciCBIBR1	= 0x34,		/* I/O base */
	PciCBILR1	= 0x38,		/* I/O limit */
	PciCBSVID	= 0x40,		/* subsystem vendor ID */
	PciCBSID	= 0x42,		/* subsystem ID */
	PciCBLMBAR	= 0x44,		/* legacy mode base address */
};

typedef struct Pcisiz Pcisiz;
struct Pcisiz
{
	Pcidev*	dev;
	int	siz;
	int	bar;
};

typedef struct Pcidev Pcidev;
struct Pcidev
{
	int	tbdf;			/* type+bus+device+function */
	ushort	vid;			/* vendor ID */
	ushort	did;			/* device ID */

	ushort	pcr;

	uchar	rid;
	uchar	ccrp;
	uchar	ccru;
	uchar	ccrb;
	uchar	cls;
	uchar	ltr;

	struct {
		ulong	bar;		/* base address */
		int	size;
	} mem[6];

	struct {
		ulong	bar;	
		int	size;
	} rom;
	uchar	intl;			/* interrupt line */

	Pcidev*	list;
	Pcidev*	link;			/* next device on this bno */

	Pcidev*	bridge;			/* down a bus */
	struct {
		ulong	bar;
		int	size;
	} ioa, mema;

	int	pmrb;			/* power management register block */
};

#define PCIWINDOW	0
#define PCIWADDR(va)	(PADDR(va)+PCIWINDOW)

/*
 * Kirkwood stuff
 */

enum {
	AddrEfuse	= PHYSIO+0x1008c,
	Addrpci		= PHYSIO+0x40000,	/* for registers below */
	Addrpcibase	= PHYSIO+0x41800,	/* for registers below */
	AddrMpp		= PHYSIO+0x10000,
	AddrSdio	= PHYSIO+0x90000,
};

enum {
	Socrevz0,
	Socreva0 = 2,
	Socreva1,
};

enum {
	/* registers; if we actually use these, change to soc.pci(base)->reg */
	PciBAR0		= Addrpcibase + 4,	/* base address */
	PciBAR1		= Addrpcibase + 8,

	PciCP		= Addrpci + 0x64,	/* capabilities pointer */

	PciINTL		= Addrpci + 0x3c,	/* interrupt line */
	PciINTP		= PciINTL + 1,	/* interrupt pin */
};

/*
 * interrupt stuff
 */

enum {
	Irqlo, Irqhi, Irqbridge,
};

enum {
	/* main interrupt cause low register bit #s (LE) */
	IRQ0hisum,		/* summary of main intr high cause reg */
	IRQ0bridge,
	IRQ0h2cdoorbell,
	IRQ0c2hdoorbell,
	_IRQ0reserved0,
	IRQ0xor0chan0,
	IRQ0xor0chan1,
	IRQ0xor1chan0,
	IRQ0xor1chan1,
	IRQ0pex0int,		/* pex = pci-express */
	_IRQ0reserved1,
	IRQ0gbe0sum,
	IRQ0gbe0rx,
	IRQ0gbe0tx,
	IRQ0gbe0misc,
	IRQ0gbe1sum,
	IRQ0gbe1rx,
	IRQ0gbe1tx,
	IRQ0gbe1misc,
	IRQ0usb0,
	_IRQ0reserved2,
	IRQ0sata,
	IRQ0crypto,
	IRQ0spi,
	IRQ0audio,
	_IRQ0reserved3,
	IRQ0ts0,
	_IRQ0reserved4,
	IRQ0sdio,
	IRQ0twsi,
	IRQ0avb,
	IRQ0tdm,

	/* main interrupt cause high register bit #s (LE) */
	_IRQ1reserved0 = 0,
	IRQ1uart0,
	IRQ1uart1,
	IRQ1gpiolo0,
	IRQ1gpiolo1,
	IRQ1gpiolo2,
	IRQ1gpiolo3,
	IRQ1gpiohi0,
	IRQ1gpiohi1,
	IRQ1gpiohi2,
	IRQ1gpiohi3,
	IRQ1xor0err,
	IRQ1xor1err,
	IRQ1pex0err,
	_IRQ1reserved1,
	IRQ1gbe0err,
	IRQ1gbe1err,
	IRQ1usberr,
	IRQ1cryptoerr,
	IRQ1audioerr,
	_IRQ1reserved2,
	_IRQ1reserved3,
	IRQ1rtc,

	/* bridged-interrupt causes */
	IRQcpuself = 0,
	IRQcputimer0,
	IRQcputimer1,
	IRQcputimerwd,
	IRQaccesserr,
};

/*
 * interrupt controller
 */
typedef struct IntrReg IntrReg;
struct IntrReg
{
	struct {
		ulong	irq;		/* main intr cause reg (ro) */
		ulong	irqmask;
		ulong	fiqmask;
		ulong	epmask;
	} lo, hi;
};

/*
 * CPU control & status (archkw.c and trap.c)
 */
typedef struct CpucsReg CpucsReg;
struct CpucsReg
{
	ulong	cpucfg;
	ulong	cpucsr;
	ulong	rstout;
	ulong	softreset;
	ulong	irq;		/* mbus(-l) bridge interrupt cause */
	ulong	irqmask;	/* ⋯ mask */
	ulong	mempm;		/* memory power mgmt. control */
	ulong	clockgate;	/* clock enable bits */
	ulong	biu;
	ulong	pad0;
	ulong	l2cfg;		/* turn l2 cache on or off, set coherency */
	ulong	pad1[2];
	ulong	l2tm0;
	ulong	l2tm1;
	ulong	pad2[2];
	ulong	l2pm;
	ulong	ram0;
	ulong	ram1;
	ulong	ram2;
	ulong	ram3;
};

enum {
	/* cpucfg bits */
	Cfgvecinithi	= 1<<1,	/* boot at 0xffff0000, not 0; default 1 */
	Cfgbigendreset	= 3<<1,	/* init. as big-endian at reset; default 0 */
	Cfgiprefetch	= 1<<16,	/* instruction prefetch enable */
	Cfgdprefetch	= 1<<17,	/* data prefetch enable */

	/* cpucsr bits */
	Reset		= 1<<1,		/* reset cpu core */

	/* rstout bits */
	RstoutPex	= 1<<0,		/* assert RSTOUTn at pci-e reset */
	RstoutWatchdog	= 1<<1,		/* assert RSTOUTn at watchdog timeout */
	RstoutSoft	= 1<<2,		/* assert RSTOUTn at sw reset */

	/* softreset bits */
	ResetSystem	= 1<<0,		/* assert RSTOUTn pin on SoftRstOutEn */

	/* l2cfg bits */
	L2ecc		= 1<<2,
	L2exists	= 1<<3,		/* l2 cache doesn't ignore cpu */
	L2writethru	= 1<<4,		/* always WT, else see PTE C & B */
};

enum {
	/* from 88f6281 func'l specs (MV-S104860-00), tables 2 & 3, chapter 2 */
	Targdram	= 0,		/* ddr sdram */
	Targflash	= 1,
	Targcesasram	= 3,		/* security accelerator sram */

	/* attributes */
	Attrcs0		= 0xe,		/* chip select 0 (low dram) */
	Attrcs1		= 0xd,		/* chip select 1 (high dram) */
	Attrbootrom	= 0x1d,
	Attrspi		= 0x1e,
	Attrnand	= 0x2f,

	Winenable	= 1<<0,
};

typedef struct Pciex Pciex;
struct Pciex {
	ushort	venid;			/* 0x11ab means Marvell */
	ushort	devid;			/* 0x6281 means 6281 */
	ulong	csr;
	ulong	revid;
	ulong	bistcache;		/* bist hdr type & cache-line size */
	ulong	bar0;
	ulong	bar0hi;
	ulong	bar1;
	ulong	bar1hi;
	ulong	bar2;
	ulong	bar2hi;
	ulong	_pad0;
	ushort	ssvenid;		/* 0x11ab means Marvell */
	ushort	ssdevid;		/* 0x11ab means Marvell */
	ulong	rombar;
	ulong	caplist;
	ulong	_pad1;
	ulong	intrpinline;		/* interrupt pin & line */
	ulong	pmcap;			/* power mgmt. capability header */
	ulong	pmcsr;			/* power mgmt. control & status */
	ulong	_pad2[2];
	ulong	msictl;			/* msi message control */
	ulong	msiaddr;
	ulong	msiaddrhi;
	ulong	msidata;
	ulong	cap;
	ulong	devcap;
	ulong	devcsr;
	ulong	linkcap;
	ulong	linkcsr;

	uchar	_pad[0x40100-0x40074];
	ulong	errrep;			/* advanced error report header */
	ulong	uncorrerr;		/* uncorrectable error status */
	ulong	uncorrerrmask;		/* uncorrectable error mask */
	ulong	uncorrerrsev;		/* uncorrectable error severity */
	ulong	correrr;		/* correctable error status */
	ulong	correrrmask;		/* correctable error mask */
	ulong	errcap;			/* advanced error capability & ctl. */
	ulong	hdrlog[4];		/* header log */
	/* continues with more rubbish at 0x41a00.  some day... */
};
