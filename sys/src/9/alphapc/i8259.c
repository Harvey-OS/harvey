#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 *  8259 interrupt controllers
 */
enum
{
	Int0ctl=	0x20,		/* control port (ICW1, OCW2, OCW3) */
	Int0aux=	0x21,		/* everything else (ICW2, ICW3, ICW4, OCW1) */
	Int1ctl=	0xA0,		/* control port */
	Int1aux=	0xA1,		/* everything else (ICW2, ICW3, ICW4, OCW1) */

	Icw1=		0x10,		/* select bit in ctl register */
	Ocw2=		0x00,
	Ocw3=		0x08,

	EOI=		0x20,		/* non-specific end of interrupt */

	Elcr1=		0x4D0,		/* Edge/Level Triggered Register */
	Elcr2=		0x4D1,
};

static int int0mask;			/* interrupts enabled for first 8259 */
static int int1mask;			/* interrupts enabled for second 8259 */

int elcr;				/* mask of level-triggered interrupts */

void
i8259init(void)
{
	int /*elcr1, */ x;

	ioalloc(Int0ctl, 2, 0, "i8259.0");
	ioalloc(Int1ctl, 2, 0, "i8259.1");
	int0mask = 0xFF;
	int1mask = 0xFF;

	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int0ctl, (1<<4)|(0<<3)|(1<<0));	/* ICW1 - master, edge triggered,
					  	   ICW4 will be sent */
	outb(Int0aux, VectorPIC);		/* ICW2 - interrupt vector offset */
	outb(Int0aux, 0x04);			/* ICW3 - have slave on level 2 */
	outb(Int0aux, 0x01);			/* ICW4 - 8086 mode, not buffered */

	/*
	 *  Set up the second 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector VectorPIC+8.
	 *  Set the 8259 as slave with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int1ctl, (1<<4)|(0<<3)|(1<<0));	/* ICW1 - master, edge triggered,
					  	   ICW4 will be sent */
	outb(Int1aux, VectorPIC+8);		/* ICW2 - interrupt vector offset */
	outb(Int1aux, 0x02);			/* ICW3 - I am a slave on level 2 */
	outb(Int1aux, 0x01);			/* ICW4 - 8086 mode, not buffered */
	outb(Int1aux, int1mask);

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	int0mask &= ~0x04;
	outb(Int0aux, int0mask);

	/*
	 * Set Ocw3 to return the ISR when ctl read.
	 * After initialisation status read is set to IRR.
	 * Read IRR first to possibly deassert an outstanding
	 * interrupt.
	 */
	x = inb(Int0ctl); USED(x);
	outb(Int0ctl, Ocw3|0x03);
	x = inb(Int1ctl); USED(x);
	outb(Int1ctl, Ocw3|0x03);

	/*
	 * Check for Edge/Level register.
	 * This check may not work for all chipsets.
	 */
/*	elcr1 = inb(Elcr1);
	outb(Elcr1, 0);
	if(inb(Elcr1) == 0){
		outb(Elcr1, 0x20);
		if(inb(Elcr1) == 0x20)
			elcr = (inb(Elcr2)<<8)|elcr1;
	}
	outb(Elcr1, elcr1);
	if(elcr)
		iprint("ELCR: %4.4uX\n", elcr);
/**/
}

int
i8259isr(int v)
{
	int isr;

	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	isr = 0;
	if(v >= VectorPIC && v <= MaxVectorPIC){
		isr = inb(Int0ctl);
		outb(Int0ctl, EOI);
		if(v >= VectorPIC+8){
			isr |= inb(Int1ctl)<<8;
			outb(Int1ctl, EOI);
		}
	}

	return isr & (1<<(v-VectorPIC));
}

int
i8259enable(int v, int, Vctl* vctl)
{
	if(v > MaxIrqPIC){
		print("i8259enable: vector %d out of range\n", v);
		return -1;
	}

	/*
	 *  enable corresponding interrupt in 8259
	 */
	if(v < 8){
		int0mask &= ~(1<<v);
		outb(Int0aux, int0mask);
	}
	else{
		int1mask &= ~(1<<(v-8));
		outb(Int1aux, int1mask);
	}

	if(elcr & (1<<v))
		vctl->eoi = i8259isr;
	else
		vctl->isr = i8259isr;
	vctl->isintr = 1;

	return v;
}
