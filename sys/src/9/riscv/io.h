/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	IrqTIMER	= 5,
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

