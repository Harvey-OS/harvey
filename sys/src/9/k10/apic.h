/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	Lock l;					/* IOAPIC: register access */
	uint32_t*	addr;				/* IOAPIC: register base */
	int	nrdt;				/* IOAPIC: size of RDT */
	int	gsib;				/* IOAPIC: global RDT index */
};

struct Lapic {
	int	machno;				/* APIC */

	uint32_t	lvt[6];
	int	nlvt;
	int	ver;

	int64_t	hz;				/* APIC Timer frequency */
	int64_t	max;
	int64_t	min;
	int64_t	div;
};

struct Apic {
	int	useable;			/* en */
	Ioapic;
	Lapic;
};

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
	Rirr		= 0x00004000,		/* Remote IRR */
	TMedge		= 0x00000000,		/* Trigger Mode Edge */
	TMlevel		= 0x00008000,		/* Trigger Mode Level */
	Im		= 0x00010000,		/* Interrupt Mask */
};

extern	Apic	xlapic[Napic];
extern	Apic	xioapic[Napic];
extern	Mach	*xlapicmachptr[Napic];		/* maintained, but unused */

#define l16get(p)	(((p)[1]<<8)|(p)[0])
#define	l32get(p)	(((uint32_t)l16get(p+2)<<16)|l16get(p))
#define	l64get(p)	(((uint64_t)l32get(p+4)<<32)|l32get(p))

extern void apicdump(void);
extern void apictimerenab(void);
extern void ioapicdump(void);

extern int pcimsienable(Pcidev*, uint64_t);
extern int pcimsimask(Pcidev*, int);
