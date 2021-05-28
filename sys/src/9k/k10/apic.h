/*
 * There are 2 flavours of APIC, Local APIC and IOAPIC,
 * which don't necessarily share one APIC ID space.
 * Each IOAPIC has a unique address, Local APICs are all
 * at the same address as they can only be accessed by
 * the local CPU.
To do: probably split this into 2 structs, Apic and IOApic;
	x2APIC...
 */
typedef struct {
	int	useable;			/* en */

	Lock;					/* IOAPIC: register access */
	u32int*	addr;				/* IOAPIC: register base */
	int	nrdt;				/* IOAPIC: size of RDT */
	int	gsib;				/* IOAPIC: global RDT index */

	int	machno;				/* APIC */
	u32int	lvt[10];
	int	nlvt;
	int	ver;				/* unused */

	vlong	hz;				/* APIC Timer frequency */
	vlong	max;
	vlong	min;
	vlong	div;
} Apic;

enum {
	Nbus		= 256,
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
	Rirr		= 0x00004000,		/* Remote IRR Status */
	TMedge		= 0x00000000,		/* Trigger Mode Edge */
	TMlevel		= 0x00008000,		/* Trigger Mode Level */
	Im		= 0x00010000,		/* Interrupt Mask */
};

Apic xapic[Napic];
Apic ioapic[Napic];

#define l16get(p)	(((p)[1]<<8)|(p)[0])
#define	l32get(p)	(((u32int)l16get(p+2)<<16)|l16get(p))
#define	l64get(p)	(((u64int)l32get(p+4)<<32)|l32get(p))

extern void apicdump(void);
extern void ioapicdump(void);
