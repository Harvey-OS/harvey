#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "io.h"

/*
 * 8259 Interrupt Controller and compatibles.
 */
enum {					/* I/O ports */
	Cntrl1		= 0x20,
	Cntrl2		= 0xa0,

	Icw1		= 0,		/* Initialisation Command Word 1 */
	Icw2		= 1,
	Icw3		= 1,
	Icw4		= 1,

	Ocw1		= 1,		/* Operational Control Word 1 */
	Ocw2		= 0,
	Ocw3		= 0,

	Imr		= Ocw1,		/* Interrupt Mask Register */
	Isr		= Ocw3,		/* In-Service Register */
	Irr		= Ocw3,		/* Interrupt Request Register */

	Elcr1		= 0x4d0,	/* Edge/Level Control Register */
	Elcr2		= 0x4d1,
};

enum {					/* Icw1 */
	Ic4		= 0x01,		/* there will be an Icw4 */
	Icw1sel		= 0x10,		/* Icw/Ocw select */
};

enum {					/* Icw3 */
	Cascaded	= 0x04,		/* Cntrl1 - Cascaded Mode Enable */
	SlaveIRQ2	= 0x02,		/* Cntrl2 - Slave Identification Code */
};

enum {					/* Icw4 */
	Microprocessor	= 0x01,		/* 80x86-based system */
};

enum {					/* Ocw2 */
	Ocw2sel		= 0x00,		/* Ocw2 select */
	Eoi		= 0x20,		/* Non-spcific EOI command */
};

enum {					/* Ocw3 */
	Irrread		= 0x02,		/* Read IRQ register */
	Isrread		= 0x03,		/* Read IS register */
	Ocw3sel		= 0x08,		/* Ocw3 select */
};

static Lock i8259lock;
static int i8259mask = ~0;		/* mask of disabled interrupts */
static int i8259elcr;			/* mask of level interrupts */

int
i8259init(int vectorbase)
{
	int elcr;

	vectorbase &= ~0x07;

	ilock(&i8259lock);

	/*
	 * Boilerplate to initialise the pair of 8259 controllers,
	 * see one of the Intel bridge datasheets for details,
	 * e.g. 82371AB (PIIX4). The default settings are 80x86 mode,
	 * edge-sensitive detection, normal EOI, non-buffered and
	 * cascade mode. Cntrl1 is connected as the master and Cntrl2
	 * as the slave; IRQ2 is used to cascade the two controllers.
	 */
	outb(Cntrl1+Icw1, Icw1sel|Ic4);
	outb(Cntrl1+Icw2, vectorbase);
	outb(Cntrl1+Icw3, Cascaded);
	outb(Cntrl1+Icw4, Microprocessor);

	outb(Cntrl2+Icw1, Icw1sel|Ic4);
	outb(Cntrl2+Icw2, vectorbase+8);
	outb(Cntrl2+Icw3, SlaveIRQ2);
	outb(Cntrl2+Icw4, Microprocessor);

	/*
	 * Set the interrupt masks, allowing interrupts
	 * to pass from Cntrl2 to Cntrl1 on IRQ2.
	 */
	i8259mask &= ~(1<<2);
	outb(Cntrl2+Imr, (i8259mask>>8) & 0xff);
	outb(Cntrl1+Imr, i8259mask & 0xff);

	outb(Cntrl1+Ocw2, Ocw2sel|Eoi);
	outb(Cntrl2+Ocw2, Ocw2sel|Eoi);

	/*
	 * Set Ocw3 to return the ISR when read for i8259isr()
	 * (after initialisation status read is set to return the IRR).
	 * Read IRR first to possibly deassert an outstanding
	 * interrupt.
	 */
	inb(Cntrl1+Irr);
	outb(Cntrl1+Ocw3, Ocw3sel|Isrread);
	inb(Cntrl2+Irr);
	outb(Cntrl2+Ocw3, Ocw3sel|Isrread);

	/*
	 * Check for Edge/Level Control register.
	 * This check may not work for all chipsets.
	 * First try a non-intrusive test - the bits for
	 * IRQs 13, 8, 2, 1 and 0 must be edge (0). If
	 * that's OK try a R/W test.
	 */
	elcr = (inb(Elcr2)<<8)|inb(Elcr1);
	if(!(elcr & 0x2107)){
		outb(Elcr1, 0);
		if(inb(Elcr1) == 0){
			outb(Elcr1, 0x20);
			if(inb(Elcr1) == 0x20)
				i8259elcr = elcr;
			outb(Elcr1, elcr & 0xff);
		}
	}
	iunlock(&i8259lock);

	return vectorbase;
}
