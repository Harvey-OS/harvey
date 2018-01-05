/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	VectorNMI	= 2,		/* non-maskable interrupt */
	VectorBPT	= 3,		/* breakpoint */
	VectorUD	= 6,		/* invalid opcode exception */
	VectorCNA	= 7,		/* coprocessor not available */
	Vector2F	= 8,		/* double fault */
	VectorCSO	= 9,		/* coprocessor segment overrun */
	VectorPF	= 14,		/* page fault */
	Vector15	= 15,		/* reserved */
	VectorCERR	= 16,		/* coprocessor error */

	VectorPIC	= 32,		/* external i8259 interrupts */
	IrqCLOCK	= 0,
	IrqKBD		= 1,
	IrqUART1	= 3,
	IrqUART0	= 4,
	IrqPCMCIA	= 5,
	IrqFLOPPY	= 6,
	IrqLPT		= 7,
	IrqIRQ7		= 7,
	IrqAUX		= 12,		/* PS/2 port */
	IrqIRQ13	= 13,		/* coprocessor on 386 */
	IrqATA0		= 14,
	IrqATA1		= 15,
	MaxIrqPIC	= 15,

	VectorLAPIC	= VectorPIC+16,	/* local APIC interrupts */
	IrqLINT0	= VectorLAPIC+0,
	IrqLINT1	= VectorLAPIC+1,
	IrqTIMER	= VectorLAPIC+2,
	IrqERROR	= VectorLAPIC+3,
	IrqPCINT	= VectorLAPIC+4,
	IrqSPURIOUS	= VectorLAPIC+15,
	MaxIrqLAPIC	= VectorLAPIC+15,

	VectorSYSCALL	= 64,

	VectorAPIC	= 65,		/* external APIC interrupts */
	MaxVectorAPIC	= 255,
};

enum {
	IdtPIC		= 32,			/* external i8259 interrupts */

	IdtLINT0	= 48,			/* local APIC interrupts */
	IdtLINT1	= 49,
	IdtTIMER	= 50,
	IdtERROR	= 51,
	IdtPCINT	= 52,

	IdtIPI		= 62,
	IdtSPURIOUS	= 63,

	IdtSYSCALL	= 64,

	IdtIOAPIC	= 65,			/* external APIC interrupts */

	IdtMAX		= 255,
};

typedef struct Vkey {
	int	tbdf;			/* pci: ioapic or msi sources */
	int	irq;			/* 8259-emulating sources */
} Vkey;

typedef struct Vctl {
	Vctl*	next;			/* handlers on this vector */

	int	isintr;			/* interrupt or fault/trap */

	Vkey Vkey;			/* source-specific key; tbdf for pci */
	void	(*f)(Ureg*, void*);	/* handler to call */
	void*	a;			/* argument to call it with */
	char	name[KNAMELEN];		/* of driver */
	char	*type;

	int	(*isr)(int);		/* get isr bit for this irq */
	int	(*eoi)(int);		/* eoi */
	int	(*mask)(Vkey*, int);	/* interrupt enable returns masked vector */
	int	vno;
} Vctl;

typedef struct ACVctl {
	char*	(*f)(Ureg*,void*);
	void*	a;
	int	vno;
	char	name[KNAMELEN];		/* of driver */
} ACVctl;

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

#define MKBUS(t,b,d,f)	(((t)<<24)|(((b)&0xFF)<<16)|(((d)&0x1F)<<11)|(((f)&0x07)<<8))
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)
#define BUSUNKNOWN	(-1)

enum {
	MaxEISA		= 16,
	CfgEISA		= 0xC80,
};

/*
 * PCI support code.
 */
enum {					/* type 0 and type 1 pre-defined header */
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
	Pciscsata	= 6,		/* SATA */

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

/* capabilities */
enum {
	PciCapPMG	= 0x01,		/* power management */
	PciCapAGP	= 0x02,
	PciCapVPD	= 0x03,		/* vital product data */
	PciCapSID	= 0x04,		/* slot id */
	PciCapMSI	= 0x05,
	PciCapCHS	= 0x06,		/* compact pci hot swap */
	PciCapPCIX	= 0x07,
	PciCapHTC	= 0x08,		/* hypertransport irq conf */
	PciCapVND	= 0x09,		/* vendor specific information */
	PciCapPCIe	= 0x10,
	PciCapMSIX	= 0x11,
	PciCapSATA	= 0x12,
	PciCapHSW	= 0x0c,		/* hot swap */
};

typedef struct Pcisiz Pcisiz;
struct Pcisiz
{
	Pcidev*	dev;
	int	siz;
	int	bar;
};

/*
 * DMG 06/11/2016 Add the definitions for generic PCI capability structure
 * based on the VIRTIO spec 1.0. Below is the layout of a capability in the PCI
 * device config space. The structure visible to the kernel repeats this, but
 * adds fields to build the linked list of capabilities per device.
 *
 * struct virtio_pci_cap {
 *   u8 cap_vndr; // Generic PCI field: PCI_CAP_ID_VNDR 
 *   u8 cap_next; // Generic PCI field: next ptr. 
 *   u8 cap_len; // Generic PCI field: capability length 
 *   u8 cfg_type; // Identifies the structure. 
 *   u8 bar; // Where to find it. 
 *   u8 padding[3]; // Pad to full dword. 
 *   le32 offset; // Offset within bar. 
 *   le32 length; // Length of the structure, in bytes. 
 * };
 * 
 * Add the linked list of capabilities to the PCI device descriptor structure.
 * 
 */

enum {
	PciCapVndr 	= 0x00,
	PciCapNext 	= 0x01,
	PciCapLen 	= 0x02,
	PciCapType	= 0x03,
	PciCapBar	= 0x04,
	PciCapOff	= 0x08,
	PciCapLength	= 0x0C
};

struct Pcidev;

typedef struct Pcicap Pcicap;
struct Pcicap {
	struct Pcidev *dev;				/* link to the device structure */
	Pcicap *link;				/* next capability or NULL */
	uint8_t vndr;				/* vendor code */
	uint8_t caplen;				/* length in the config area */
	uint8_t type;				/* capability config type */
	uint8_t bar;				/* BAR index in the device structure 0 - 5 */
	uint32_t offset;			/* offset within BAR */
	uint32_t length;			/* length in the memory or IO space */
};

typedef struct Pcidev Pcidev;
struct Pcidev
{
	int	tbdf;			/* type+bus+device+function */
	uint16_t	vid;			/* vendor ID */
	uint16_t	did;			/* device ID */

	uint16_t	pcr;

	unsigned char	rid;
	unsigned char	ccrp;
	unsigned char	ccru;
	unsigned char	ccrb;
	unsigned char	cls;
	unsigned char	ltr;

	struct {
		uint32_t	bar;		/* base address */
		int	size;
	} mem[6];

	struct {
		uint32_t	bar;
		int	size;
	} rom;
	unsigned char	intl;			/* interrupt line */

	Pcidev*	list;
	Pcidev*	link;			/* next device on this bno */

	Pcidev*	bridge;			/* down a bus */
	struct {
		uint32_t	bar;
		int	size;
	} ioa, mema;
	Pcicap *caplist;
	uint32_t capcnt;
	Pcicap **capidx;
};

#define PCIWINDOW	0
#define PCIWADDR(va)	(PADDR(va)+PCIWINDOW)
#define ISAWINDOW	0
#define ISAWADDR(va)	(PADDR(va)+ISAWINDOW)

#pragma	varargck	type	"T"	int
