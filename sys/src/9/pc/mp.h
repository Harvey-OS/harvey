/*
 * MultiProcessor Specification Version 1.[14].
 */
typedef struct {			/* floating pointer */
	uchar	signature[4];		/* "_MP_" */
	long	physaddr;		/* physical address of MP configuration table */
	uchar	length;			/* 1 */
	uchar	specrev;		/* [14] */
	uchar	checksum;		/* all bytes must add up to 0 */
	uchar	type;			/* MP system configuration type */
	uchar	imcrp;
	uchar	reserved[3];
} _MP_;

typedef struct {			/* configuration table header */
	uchar	signature[4];		/* "PCMP" */
	ushort	length;			/* total table length */
	uchar	version;		/* [14] */
	uchar	checksum;		/* all bytes must add up to 0 */
	uchar	product[20];		/* product id */
	ulong	oemtable;		/* OEM table pointer */
	ushort	oemlength;		/* OEM table length */
	ushort	entry;			/* entry count */
	ulong	lapicbase;		/* address of local APIC */
	ushort	xlength;		/* extended table length */
	uchar	xchecksum;		/* extended table checksum */
	uchar	reserved;
} PCMP;

typedef struct {			/* processor table entry */
	uchar	type;			/* entry type (0) */
	uchar	apicno;			/* local APIC id */
	uchar	version;		/* local APIC verison */
	uchar	flags;			/* CPU flags */
	uchar	signature[4];		/* CPU signature */
	ulong	feature;		/* feature flags from CPUID instruction */
	uchar	reserved[8];
} PCMPprocessor;

typedef struct {			/* bus table entry */
	uchar	type;			/* entry type (1) */
	uchar	busno;			/* bus id */
	char	string[6];		/* bus type string */
} PCMPbus;

typedef struct {			/* I/O APIC table entry */
	uchar	type;			/* entry type (2) */
	uchar	apicno;			/* I/O APIC id */
	uchar	version;		/* I/O APIC version */
	uchar	flags;			/* I/O APIC flags */
	ulong	addr;			/* I/O APIC address */
} PCMPioapic;

typedef struct {			/* interrupt table entry */
	uchar	type;			/* entry type ([34]) */
	uchar	intr;			/* interrupt type */
	ushort	flags;			/* interrupt flag */
	uchar	busno;			/* source bus id */
	uchar	irq;			/* source bus irq */
	uchar	apicno;			/* destination APIC id */
	uchar	intin;			/* destination APIC [L]INTIN# */
} PCMPintr;

typedef struct {			/* system address space mapping entry */
	uchar	type;			/* entry type (128) */
	uchar	length;			/* of this entry (20) */
	uchar	busno;			/* bus id */
	uchar	addrtype;
	ulong	addrbase[2];
	ulong	addrlength[2];
} PCMPsasm;

typedef struct {			/* bus hierarchy descriptor entry */
	uchar	type;			/* entry type (129) */
	uchar	length;			/* of this entry (8) */
	uchar	busno;			/* bus id */
	uchar	info;			/* bus info */
	uchar	parent;			/* parent bus */
	uchar	reserved[3];
} PCMPhierarchy;

typedef struct {			/* compatibility bus address space modifier entry */
	uchar	type;			/* entry type (130) */
	uchar	length;			/* of this entry (8) */
	uchar	busno;			/* bus id */
	uchar	modifier;		/* address modifier */
	ulong	range;			/* predefined range list */
} PCMPcbasm;

enum {					/* table entry types */
	PcmpPROCESSOR	= 0x00,		/* one entry per processor */
	PcmpBUS		= 0x01,		/* one entry per bus */
	PcmpIOAPIC	= 0x02,		/* one entry per I/O APIC */
	PcmpIOINTR	= 0x03,		/* one entry per bus interrupt source */
	PcmpLINTR	= 0x04,		/* one entry per system interrupt source */

	PcmpSASM	= 0x80,
	PcmpHIERARCHY	= 0x81,
	PcmpCBASM	= 0x82,

					/* PCMPprocessor and PCMPioapic flags */
	PcmpEN		= 0x01,		/* enabled */
	PcmpBP		= 0x02,		/* bootstrap processor */
	PcmpUsed	= 1ul<<31,	/* Apic->flags addition */

					/* PCMPiointr and PCMPlintr flags */
	PcmpPOMASK	= 0x03,		/* polarity conforms to specifications of bus */
	PcmpHIGH	= 0x01,		/* active high */
	PcmpLOW		= 0x03,		/* active low */
	PcmpELMASK	= 0x0C,		/* trigger mode of APIC input signals */
	PcmpEDGE	= 0x04,		/* edge-triggered */
	PcmpLEVEL	= 0x0C,		/* level-triggered */

					/* PCMPiointr and PCMPlintr interrupt type */
	PcmpINT		= 0x00,		/* vectored interrupt from APIC Rdt */
	PcmpNMI		= 0x01,		/* non-maskable interrupt */
	PcmpSMI		= 0x02,		/* system management interrupt */
	PcmpExtINT	= 0x03,		/* vectored interrupt from external PIC */

					/* PCMPsasm addrtype */
	PcmpIOADDR	= 0x00,		/* I/O address */
	PcmpMADDR	= 0x01,		/* memory address */
	PcmpPADDR	= 0x02,		/* prefetch address */

					/* PCMPhierarchy info */
	PcmpSD		= 0x01,		/* subtractive decode bus */

					/* PCMPcbasm modifier */
	PcmpPR		= 0x01,		/* predefined range list */
};

/*
 * Condensed form of the MP Configuration Table.
 * This is created during a single pass through the MP Configuration
 * table.
 */
typedef struct Aintr Aintr;
typedef struct Apic Apic;
typedef struct Bus Bus;

typedef struct Bus {
	uchar	type;
	uchar	busno;
	uchar	po;
	uchar	el;

	Aintr*	aintr;			/* interrupts tied to this bus */
	Bus*	next;
} Bus;

typedef struct Aintr {
	PCMPintr* intr;
	Apic*	apic;
	Aintr*	next;
};

struct Apic {
	int	type;
	int	apicno;
	ulong*	addr;			/* register base address */
	ulong	paddr;
	int	flags;			/* PcmpBP|PcmpEN */

	Lock;				/* I/O APIC: register access */
	int	mre;			/* I/O APIC: maximum redirection entry */

	int	lintr[2];		/* Local APIC */
	int	machno;

	int	online;
};

enum {
	/*
	 * global lapicbase is a kernel virtual addr.;
	 * we need the physical addr. in the msi addr. reg.
	 */
	Lapicphys = 0xfee00000ul,
};

enum {
	MaxAPICNO	= 254,		/* 255 is physical broadcast */
};

enum {					/* I/O APIC registers */
	IoapicID	= 0x00,		/* ID */
	IoapicVER	= 0x01,		/* version */
	IoapicARB	= 0x02,		/* arbitration ID */
	IoapicRDT	= 0x10,		/* redirection table */
};

/*
 * Common bits for
 *	I/O APIC Redirection Table Entry;
 *	Local APIC Local Interrupt Vector Table;
 *	Local APIC Inter-Processor Interrupt;
 *	Local APIC Timer Vector Table.
 */
enum {
	ApicFIXED	= 0x00000000,	/* [10:8] Delivery Mode */
	ApicLOWEST	= 0x00000100,	/* Lowest priority */
	ApicSMI		= 0x00000200,	/* System Management Interrupt */
	ApicRR		= 0x00000300,	/* Remote Read */
	ApicNMI		= 0x00000400,
	ApicINIT	= 0x00000500,	/* INIT/RESET */
	ApicSTARTUP	= 0x00000600,	/* Startup IPI */
	ApicExtINT	= 0x00000700,

	ApicPHYSICAL	= 0x00000000,	/* [11] Destination Mode (RW) */
	ApicLOGICAL	= 0x00000800,

	ApicDELIVS	= 0x00001000,	/* [12] Delivery Status (RO) */
	ApicHIGH	= 0x00000000,	/* [13] Interrupt Input Pin Polarity (RW) */
	ApicLOW		= 0x00002000,
	ApicRemoteIRR	= 0x00004000,	/* [14] Remote IRR (RO) */
	ApicEDGE	= 0x00000000,	/* [15] Trigger Mode (RW) */
	ApicLEVEL	= 0x00008000,
	ApicIMASK	= 0x00010000,	/* [16] Interrupt Mask */
};

enum {					/* Local APIC registers */
	LapicID		= 0x0020,	/* ID */
	LapicVER	= 0x0030,	/* Version */
	LapicTPR	= 0x0080,	/* Task Priority */
	LapicAPR	= 0x0090,	/* Arbitration Priority */
	LapicPPR	= 0x00A0,	/* Processor Priority */
	LapicEOI	= 0x00B0,	/* EOI */
	LapicLDR	= 0x00D0,	/* Logical Destination */
	LapicDFR	= 0x00E0,	/* Destination Format */
	LapicSVR	= 0x00F0,	/* Spurious Interrupt Vector */
	LapicISR	= 0x0100,	/* Interrupt Status (8 registers) */
	LapicTMR	= 0x0180,	/* Trigger Mode (8 registers) */
	LapicIRR	= 0x0200,	/* Interrupt Request (8 registers) */
	LapicESR	= 0x0280,	/* Error Status */
	LapicICRLO	= 0x0300,	/* Interrupt Command */
	LapicICRHI	= 0x0310,	/* Interrupt Command [63:32] */
	LapicTIMER	= 0x0320,	/* Local Vector Table 0 (TIMER) */
	LapicPCINT	= 0x0340,	/* Performance Counter LVT */
	LapicLINT0	= 0x0350,	/* Local Vector Table 1 (LINT0) */
	LapicLINT1	= 0x0360,	/* Local Vector Table 2 (LINT1) */
	LapicERROR	= 0x0370,	/* Local Vector Table 3 (ERROR) */
	LapicTICR	= 0x0380,	/* Timer Initial Count */
	LapicTCCR	= 0x0390,	/* Timer Current Count */
	LapicTDCR	= 0x03E0,	/* Timer Divide Configuration */
};

enum {					/* LapicSVR */
	LapicENABLE	= 0x00000100,	/* Unit Enable */
	LapicFOCUS	= 0x00000200,	/* Focus Processor Checking Disable */
};

enum {					/* LapicICRLO */
					/* [14] IPI Trigger Mode Level (RW) */
	LapicDEASSERT	= 0x00000000,	/* Deassert level-sensitive interrupt */
	LapicASSERT	= 0x00004000,	/* Assert level-sensitive interrupt */

					/* [17:16] Remote Read Status */
	LapicINVALID	= 0x00000000,	/* Invalid */
	LapicWAIT	= 0x00010000,	/* In-Progress */
	LapicVALID	= 0x00020000,	/* Valid */

					/* [19:18] Destination Shorthand */
	LapicFIELD	= 0x00000000,	/* No shorthand */
	LapicSELF	= 0x00040000,	/* Self is single destination */
	LapicALLINC	= 0x00080000,	/* All including self */
	LapicALLEXC	= 0x000C0000,	/* All Excluding self */
};

enum {					/* LapicESR */
	LapicSENDCS	= 0x00000001,	/* Send CS Error */
	LapicRCVCS	= 0x00000002,	/* Receive CS Error */
	LapicSENDACCEPT	= 0x00000004,	/* Send Accept Error */
	LapicRCVACCEPT	= 0x00000008,	/* Receive Accept Error */
	LapicSENDVECTOR	= 0x00000020,	/* Send Illegal Vector */
	LapicRCVVECTOR	= 0x00000040,	/* Receive Illegal Vector */
	LapicREGISTER	= 0x00000080,	/* Illegal Register Address */
};

enum {					/* LapicTIMER */
					/* [17] Timer Mode (RW) */
	LapicONESHOT	= 0x00000000,	/* One-shot */
	LapicPERIODIC	= 0x00020000,	/* Periodic */

					/* [19:18] Timer Base (RW) */
	LapicCLKIN	= 0x00000000,	/* use CLKIN as input */
	LapicTMBASE	= 0x00040000,	/* use TMBASE */
	LapicDIVIDER	= 0x00080000,	/* use output of the divider */
};

enum {					/* LapicTDCR */
	LapicX2		= 0x00000000,	/* divide by 2 */
	LapicX4		= 0x00000001,	/* divide by 4 */
	LapicX8		= 0x00000002,	/* divide by 8 */
	LapicX16	= 0x00000003,	/* divide by 16 */
	LapicX32	= 0x00000008,	/* divide by 32 */
	LapicX64	= 0x00000009,	/* divide by 64 */
	LapicX128	= 0x0000000A,	/* divide by 128 */
	LapicX1		= 0x0000000B,	/* divide by 1 */
};
extern void ioapicinit(Apic*, int);
extern void ioapicrdtr(Apic*, int, int*, int*);
extern void ioapicrdtw(Apic*, int, int, int);

extern int lapicclock(Ureg*, void*);
extern int lapiceoi(int);
extern int lapicerror(Ureg*, void*);
extern void lapicicrw(ulong, ulong);
extern void lapicinit(Apic*);
extern void lapicintroff(void);
extern void lapicintron(void);
extern int lapicisr(int);
extern void lapicnmidisable(void);
extern void lapicnmienable(void);
extern void lapiconline(void);
extern int lapicspurious(Ureg*, void*);
extern void lapicstartap(Apic*, int);
extern void lapictimerset(uvlong);

extern int mpidentify(void);
extern void mpinit(void);
extern int mpintrenable(Vctl*);
extern void mpshutdown(void);

_MP_ *_mp_;

PCArch archmp;
