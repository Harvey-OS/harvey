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

static Lock i8259lock;
static int i8259mask = 0xFFFF;		/* disabled interrupts */
int i8259elcr;				/* mask of level-triggered interrupts */

void
i8259init(void)
{
	int x;

	ioalloc(Int0ctl, 2, 0, "i8259.0");
	ioalloc(Int1ctl, 2, 0, "i8259.1");
	ilock(&i8259lock);

	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector VectorPIC.
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
	outb(Int1aux, (i8259mask>>8) & 0xFF);

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	i8259mask &= ~0x04;
	outb(Int0aux, i8259mask & 0xFF);

	/*
	 * Set Ocw3 to return the ISR when ctl read.
	 * After initialisation status read is set to IRR.
	 * Read IRR first to possibly deassert an outstanding
	 * interrupt.
	 */
	inb(Int0ctl);
	outb(Int0ctl, Ocw3|0x03);
	inb(Int1ctl);
	outb(Int1ctl, Ocw3|0x03);

	/*
	 * Check for Edge/Level register.
	 * This check may not work for all chipsets.
	 * First try a non-intrusive test - the bits for
	 * IRQs 13, 8, 2, 1 and 0 must be edge (0). If
	 * that's OK try a R/W test.
	 */
	x = (inb(Elcr2)<<8)|inb(Elcr1);
	if(!(x & 0x2107)){
		outb(Elcr1, 0);
		if(inb(Elcr1) == 0){
			outb(Elcr1, 0x20);
			if(inb(Elcr1) == 0x20)
				i8259elcr = x;
			outb(Elcr1, x & 0xFF);
			print("ELCR: %4.4uX\n", i8259elcr);
		}
	}
	iunlock(&i8259lock);
}

int
i8259isr(int vno)
{
	int irq, isr;

	if(vno < VectorPIC || vno > VectorPIC+MaxIrqPIC)
		return 0;
	irq = vno-VectorPIC;

	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	ilock(&i8259lock);
	isr = inb(Int0ctl);
	outb(Int0ctl, EOI);
	if(irq >= 8){
		isr |= inb(Int1ctl)<<8;
		outb(Int1ctl, EOI);
	}
	iunlock(&i8259lock);

	return isr & (1<<irq);
}

int
i8259enable(Vctl* v)
{
	int irq, irqbit;

	/*
	 * Given an IRQ, enable the corresponding interrupt in the i8259
	 * and return the vector to be used. The i8259 is set to use a fixed
	 * range of vectors starting at VectorPIC.
	 */
	irq = v->irq;
	if(irq < 0 || irq > MaxIrqPIC){
		print("i8259enable: irq %d out of range\n", irq);
		return -1;
	}
	irqbit = 1<<irq;

	ilock(&i8259lock);
	if(!(i8259mask & irqbit) && !(i8259elcr & irqbit)){
		print("i8259enable: irq %d shared but not level\n", irq);
		iunlock(&i8259lock);
		return -1;
	}
	i8259mask &= ~irqbit;
	if(irq < 8)
		outb(Int0aux, i8259mask & 0xFF);
	else
		outb(Int1aux, (i8259mask>>8) & 0xFF);

	if(i8259elcr & irqbit)
		v->eoi = i8259isr;
	else
		v->isr = i8259isr;
	iunlock(&i8259lock);

	return VectorPIC+irq;
}

int
i8259vecno(int irq)
{
	return VectorPIC+irq;
}

int
i8259disable(int irq)
{
	int irqbit;

	/*
	 * Given an IRQ, disable the corresponding interrupt
	 * in the 8259.
	 */
	if(irq < 0 || irq > MaxIrqPIC){
		print("i8259disable: irq %d out of range\n", irq);
		return -1;
	}
	irqbit = 1<<irq;

	ilock(&i8259lock);
	if(!(i8259mask & irqbit)){
		i8259mask |= irqbit;
		if(irq < 8)
			outb(Int0aux, i8259mask & 0xFF);
		else
			outb(Int1aux, (i8259mask>>8) & 0xFF);
	}
	iunlock(&i8259lock);
	return 0;
}
