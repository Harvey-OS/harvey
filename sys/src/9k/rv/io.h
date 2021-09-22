typedef struct Vctl {
	Vctl*	next;			/* handlers on this vector */
	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;			/* argument to call it with */

	uchar	isintr;			/* flag: interrupt, else fault/trap */
	uchar	ismsi;			/* flag: is msi interrupt? */
	uchar	type;			/* exception or some form of intr? */
	short	irq;
	short	vno;
	int	tbdf;

	int	(*isr)(int);		/* get isr bit for this irq */
	int	(*eoi)(int);		/* end interrupt service: dismiss it */

	/* precise interrupt accounting */
	uvlong	count;			/* interrupt count */
	ulong	unclaimed;		/* # of interrupts unclaimed */
	ulong	intrunknown;		/* # of interrupts via intrunknown */
	int	cpu;			/* targetted cpu */

	char	name[KNAMELEN];		/* of driver */
} Vctl;

enum Buses {		/* indices for bustypes[] */
	BusINTERN,	/* Internal bus */
	BusISA,		/* Industry Standard Architecture */
	BusPCI,		/* Peripheral Component Interconnect (Express) */
};

#define MKBUS(t,b,d,f)	((t)<<24 | ((b)&0xFF)<<16 | ((d)&0x1F)<<11 | ((f)&0x07)<<8)
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)
#define BUSUNKNOWN	(-1)

/*
 * PCI support code.
 */

/* config space */
enum {					/* type 0 & 1 pre-defined header */
	PciVID		= 0x00,		/* vendor ID */
	PciDID		= 0x02,		/* device ID */
	PciPCR		= 0x04,		/* command */
	PciPSR		= 0x06,		/* status */
	PciRID		= 0x08,		/* revision ID */
	PciCCRp		= 0x09,		/* programming interface class code */
	PciCCRu		= 0x0A,		/* sub-class code */
	PciCCRb		= 0x0B,		/* base class code */
	PciCLS		= 0x0C,		/* cache line size; unused in pci-e */
	PciLTR		= 0x0D,		/* latency timer */
	PciHDT		= 0x0E,		/* header type */
	PciBST		= 0x0F,		/* BIST */

	PciBAR0		= 0x10,		/* base address */
	PciBAR1		= 0x14,

	PciCP		= 0x34,		/* capabilities pointer */

	PciINTL		= 0x3C,		/* interrupt line */
	PciINTP		= 0x3D,		/* interrupt pin */
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
	Pciscraid	= 4,		/* RAID */
	Pciscsata	= 6,		/* SATA */
	Pciscnvm	= 8,		/* non-volatile memory (flash) */
	Pciscstgeneric	= 0x80,		/* mass storage */

	/* network */
	Pciscether	= 0,		/* Ethernet */
	Pciscnetgeneric	= 0x80,		/* some network */

	/* display */
	Pciscvga	= 0,		/* VGA */
	Pciscxga	= 1,		/* XGA */
	Pcisc3d		= 2,		/* 3D */

	/* bridges */
	Pciscbrhost	= 0,		/* host (e.g., southbridge) */
	Pciscbrisa	= 1,		/* isa */
	Pciscbrpci	= 4,		/* pci */
	Pciscbrcard	= 7,		/* obsolete card bus */
	Pciscbrpci½trans= 9,		/* pci semi-transparent */
	Pciscbrgeneric	= 0x80,		/* some bridge */

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

enum {					/* PSR bits */
	Pcicap		= 1<<4,
};

enum {					/* pci 2.2 capability ids */
	Pcicappwr	= 1,		/* power management */
	Pcicapagp	= 2,		/* accelerated graphics port */
	Pcicapvpd	= 3,		/* vital product data */
	Pcicapbridge	= 4,		/* bridge info */
	Pcicapmsi	= 5,		/* message-signaled interrupts */
	Pcicaphot	= 6,		/* CompactPCI hot swap */
	Pcicappcie	= 0x10,		/* pci-express */
	Pcicapmsix	= 0x11,		/* msi-x */
};

typedef struct Pcisiz Pcisiz;
struct Pcisiz
{
	Pcidev*	dev;
	int	siz;
	int	bar;
};

typedef struct Pcibar Pcibar;
struct Pcibar {
	ulong	bar;			/* base address */
	int	size;
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

	Pcibar	mem[6];

	Pcibar	rom;
	uchar	intl;			/* interrupt line */

	Pcidev*	list;
	Pcidev*	link;			/* next device on this bno */

	Pcidev*	bridge;			/* down a bus */
	Pcibar	ioa, mema;

	/* cache config space offsets */
	short	pmrb;			/* power management register block */
	short	msiptr;			/* msi capability ptr */
	short	msixptr;		/* msi-x capability ptr */
	short	pcieptr;		/* pci-express ptr */
};

struct Msi {				/* msi pci capability */
	ushort	ctl;
	uvlong	addr;
	ushort	data;
};

enum {
	/* msi control reg. */
	Msienable	= 1<<0,
	Msimme		= 7<<4,		/* multiple message enable (count) */
	Msiaddr64	= 1<<7,		/* RO: 64-bit address register? */

	/* msi address reg. */
	Msiphysdest	= 0<<2,		/* iff RH == 1, physical mode (DM) */
	Msilogdest	= 1<<2,		/* iff RH == 1, logical mode (DM) */
	Msirhtocpuid	= 0<<3,		/* deliver to cpu lapic id (RH) */
	Msirhcomplex	= 1<<3,		/* more complex options (RH) */

	/* msi data reg. */
	Msitrglevel	= 1 << 15,	/* trigger on level */
	Msitrglvlassert	= 1 << 14,	/* trigger on level asserted */
	Msidlvfixed	= 0 << 8,	/* delivery mode: fixed */
};

enum {
	/* pci vendor ids */
	Vamd	= 0x1022,
	Vatiamd	= 0x1002,
	Vintel	= 0x8086,
	Vjmicron= 0x197b,
	Vmarvell= 0x1b4b,
	Vmyricom= 0x14c1,
	Voracle	= 0x80ee,
	Vparallels= 0x1ab8,
	Vvmware	= 0x15ad,
};

#define PCIWINDOW	0
#define PCIWADDR(va)	(PADDR(va)+PCIWINDOW)

#pragma varargck	type	"T"	int
#pragma varargck	type	"T"	uint
