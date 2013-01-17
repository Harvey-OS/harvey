/*
 * There are 2 flavours of APIC, Local APIC and IOAPIC,
 * Each I/O APIC has a unique physical address,
 * Local APICs are all at the same physical address as they can only be
 * accessed by the local CPU.  APIC ids are unique to the
 * APIC type, so an IOAPIC and APIC both with id 0 is ok.
 */
typedef	struct	Ioapic	Ioapic;
typedef	struct	Lapic	Lapic;
typedef	struct	Apic	Apic;

struct Ioapic {
	Lock;					/* register access */
	u32int*	addr;				/* register base */
	uintmem	paddr;				/* register base */
	int	nrdt;				/* size of RDT */
	int	ibase;				/* global interrupt base */
};

struct Lapic {
	int	machno;				/* APIC */

	u32int	lvt[7];
	int	nlvt;
	int	ver;

	vlong	hz;				/* APIC Timer frequency */
	vlong	max;
	vlong	min;
	vlong	div;
};

struct Apic {
	int	useable;			/* en */
	Ioapic;
	Lapic;
};

enum {
	Nbus		= 256,			/* must be 256 */
	Napic		= 254,			/* xAPIC architectural limit */
	Nrdt		= 64,
};

/*
 * Common bits for
 *	IOAPIC Redirection Table Entry (RDT);
 *	APIC Local Vector Table Entry (LVT);
 *	APIC Interrupt Command Register (ICR).
 * [10:8] Message Type
 * [11] Destination Mode (RW)
 * [12] Delivery Status (RO)
 * [13] Interrupt Input Pin Polarity (RW)
 * [14] Remote IRR (RO)
 * [15] Trigger Mode (RW)
 * [16] Interrupt Mask
 */
enum {
	MTf		= 0x00000000,		/* Fixed */
	MTlp		= 0x00000100,		/* Lowest Priority */
	MTsmi		= 0x00000200,		/* SMI */
	MTrr		= 0x00000300,		/* Remote Read */
	MTnmi		= 0x00000400,		/* NMI */
	MTir		= 0x00000500,		/* INIT/RESET */
	MTsipi		= 0x00000600,		/* Startup IPI */
	MTei		= 0x00000700,		/* ExtINT */

	Pm		= 0x00000000,		/* Physical Mode */
	Lm		= 0x00000800,		/* Logical Mode */

	Ds		= 0x00001000,		/* Delivery Status */
	IPhigh		= 0x00000000,		/* IIPP High */
	IPlow		= 0x00002000,		/* IIPP Low */
	Rirr		= 0x00004000,		/* Remote IRR */
	TMedge		= 0x00000000,		/* Trigger Mode Edge */
	TMlevel		= 0x00008000,		/* Trigger Mode Level */
	Im		= 0x00010000,		/* Interrupt Mask */
};

extern	Apic	xlapic[Napic];
extern	Apic	xioapic[Napic];
extern	Mach	*xlapicmachptr[Napic];		/* maintained, but unused */

extern void apicdump(void);
extern void apictimerenab(void);
extern void ioapicdump(void);

extern int pcimsienable(Pcidev*, uvlong);
extern int pcimsimask(Pcidev*, int);
