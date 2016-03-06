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

