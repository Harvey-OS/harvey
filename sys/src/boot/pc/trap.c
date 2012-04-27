#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

void	intr0(void), intr1(void), intr2(void), intr3(void);
void	intr4(void), intr5(void), intr6(void), intr7(void);
void	intr8(void), intr9(void), intr10(void), intr11(void);
void	intr12(void), intr13(void), intr14(void), intr15(void);
void	intr16(void);
void	intr24(void), intr25(void), intr26(void), intr27(void);
void	intr28(void), intr29(void), intr30(void), intr31(void);
void	intr32(void), intr33(void), intr34(void), intr35(void);
void	intr36(void), intr37(void), intr38(void), intr39(void);
void	intr64(void);
void	intrbad(void);

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

int	int0mask = 0xff;	/* interrupts enabled for first 8259 */
int	int1mask = 0xff;	/* interrupts enabled for second 8259 */
int i8259elcr;				/* mask of level-triggered interrupts */

/*
 *  trap/interrupt gates
 */
Segdesc ilt[256];

enum 
{
	Maxhandler=	32,		/* max number of interrupt handlers */
};

typedef struct Handler	Handler;
struct Handler
{
	void	(*r)(Ureg*, void*);
	void	*arg;
	Handler	*next;
};

struct
{
	Handler	*ivec[256];
	Handler	h[Maxhandler];
	int	nextfree;
} halloc;

void
sethvec(int v, void (*r)(void), int type, int pri)
{
	ilt[v].d0 = ((ulong)r)&0xFFFF|(KESEL<<16);
	ilt[v].d1 = ((ulong)r)&0xFFFF0000|SEGP|SEGPL(pri)|type;
}

void
setvec(int v, void (*r)(Ureg*, void*), void *arg)
{
	Handler *h;

	if(halloc.nextfree >= Maxhandler)
		panic("out of interrupt handlers");
	h = &halloc.h[halloc.nextfree++];
	h->next = halloc.ivec[v];
	h->r = r;
	h->arg = arg;
	halloc.ivec[v] = h;

	/*
	 *  enable corresponding interrupt in 8259
	 */
	if((v&~0x7) == VectorPIC){
		int0mask &= ~(1<<(v&7));
		outb(Int0aux, int0mask);
	} else if((v&~0x7) == VectorPIC+8){
		int1mask &= ~(1<<(v&7));
		outb(Int1aux, int1mask);
	}
}

void
trapdisable(void)
{
	outb(Int0aux, 0xFF);
	outb(Int1aux, 0xFF);
}

void
trapenable(void)
{
	outb(Int0aux, int0mask);
	outb(Int1aux, int1mask);
}


/*
 *  set up the interrupt/trap gates
 */
void
trapinit(void)
{
	int i, x;

	/*
	 *  set all interrupts to panics
	 */
	for(i = 0; i < 256; i++)
		sethvec(i, intrbad, SEGTG, 0);

	/*
	 *  80386 processor (and coprocessor) traps
	 */
	sethvec(0, intr0, SEGTG, 0);
	sethvec(1, intr1, SEGTG, 0);
	sethvec(2, intr2, SEGTG, 0);
	sethvec(3, intr3, SEGTG, 0);
	sethvec(4, intr4, SEGTG, 0);
	sethvec(5, intr5, SEGTG, 0);
	sethvec(6, intr6, SEGTG, 0);
	sethvec(7, intr7, SEGTG, 0);
	sethvec(8, intr8, SEGTG, 0);
	sethvec(9, intr9, SEGTG, 0);
	sethvec(10, intr10, SEGTG, 0);
	sethvec(11, intr11, SEGTG, 0);
	sethvec(12, intr12, SEGTG, 0);
	sethvec(13, intr13, SEGTG, 0);
	sethvec(14, intr14, SEGTG, 0);
	sethvec(15, intr15, SEGTG, 0);
	sethvec(16, intr16, SEGTG, 0);

	/*
	 *  device interrupts
	 */
	sethvec(24, intr24, SEGIG, 0);
	sethvec(25, intr25, SEGIG, 0);
	sethvec(26, intr26, SEGIG, 0);
	sethvec(27, intr27, SEGIG, 0);
	sethvec(28, intr28, SEGIG, 0);
	sethvec(29, intr29, SEGIG, 0);
	sethvec(30, intr30, SEGIG, 0);
	sethvec(31, intr31, SEGIG, 0);
	sethvec(32, intr32, SEGIG, 0);
	sethvec(33, intr33, SEGIG, 0);
	sethvec(34, intr34, SEGIG, 0);
	sethvec(35, intr35, SEGIG, 0);
	sethvec(36, intr36, SEGIG, 0);
	sethvec(37, intr37, SEGIG, 0);
	sethvec(38, intr38, SEGIG, 0);
	sethvec(39, intr39, SEGIG, 0);

	/*
	 *  tell the hardware where the table is (and how long)
	 */
	putidt(ilt, sizeof(ilt)-1);

	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector VectorPIC.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int0ctl, Icw1|0x01);	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int0aux, VectorPIC);	/* ICW2 - interrupt vector offset */
	outb(Int0aux, 0x04);		/* ICW3 - have slave on level 2 */
	outb(Int0aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  Set up the second 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector VectorPIC+8.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int1ctl, Icw1|0x01);	/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int1aux, VectorPIC+8);	/* ICW2 - interrupt vector offset */
	outb(Int1aux, 0x02);		/* ICW3 - I am a slave on level 2 */
	outb(Int1aux, 0x01);		/* ICW4 - 8086 mode, not buffered */
	outb(Int1aux, int1mask);

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	int0mask &= ~0x04;
	outb(Int0aux, int0mask);

	/*
	 * Set Ocw3 to return the ISR when ctl read.
	 */
	outb(Int0ctl, Ocw3|0x03);
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
}

/*
 *  dump registers
 */
static void
dumpregs(Ureg *ur)
{
	print("FLAGS=%lux TRAP=%lux ECODE=%lux PC=%lux\n",
		ur->flags, ur->trap, ur->ecode, ur->pc);
	print("  AX %8.8lux  BX %8.8lux  CX %8.8lux  DX %8.8lux\n",
		ur->ax, ur->bx, ur->cx, ur->dx);
	print("  SI %8.8lux  DI %8.8lux  BP %8.8lux\n",
		ur->si, ur->di, ur->bp);
	print("  CS %4.4lux DS %4.4lux  ES %4.4lux  FS %4.4lux  GS %4.4lux\n",
		ur->cs & 0xFF, ur->ds & 0xFFFF, ur->es & 0xFFFF, ur->fs & 0xFFFF, ur->gs & 0xFFFF);
	print("  CR0 %8.8lux CR2 %8.8lux CR3 %8.8lux\n",
		getcr0(), getcr2(), getcr3());
}

/*
 *  All traps
 */
void
trap(Ureg *ur)
{
	int v;
	int c;
	Handler *h;
	ushort isr;

	v = ur->trap;
	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	c = v&~0x7;
	isr = 0;
	if(c==VectorPIC || c==VectorPIC+8){
		isr = inb(Int0ctl);
		outb(Int0ctl, EOI);
		if(c == VectorPIC+8){
			isr |= inb(Int1ctl)<<8;
			outb(Int1ctl, EOI);
		}
	}

	if(v>=256 || (h = halloc.ivec[v]) == 0){
		if(v >= VectorPIC && v < VectorPIC+16){
			v -= VectorPIC;
			/*
			 * Check for a default IRQ7. This can happen when
			 * the IRQ input goes away before the acknowledge.
			 * In this case, a 'default IRQ7' is generated, but
			 * the corresponding bit in the ISR isn't set.
			 * In fact, just ignore all such interrupts.
			 */
			if(isr & (1<<v))
				print("unknown interrupt %d pc=0x%lux\n", v, ur->pc);
			return;
		}

		switch(v){

		case 0x02:				/* NMI */
			print("NMI: nmisc=0x%2.2ux, nmiertc=0x%2.2ux, nmiesc=0x%2.2ux\n",
				inb(0x61), inb(0x70), inb(0x461));
			return;

		default:
			dumpregs(ur);
			panic("exception/interrupt %d", v);
			return;
		}
	}

	/*
	 *  call the trap routines
	 */
	do {
		(*h->r)(ur, h->arg);
		h = h->next;
	} while(h);
}

extern void realmode0(void);	/* in l.s */

extern int realmodeintr;
extern Ureg realmoderegs;

void
realmode(int intr, Ureg *ureg)
{
	realmoderegs = *ureg;
	realmodeintr = intr;
	trapdisable();
	realmode0();
	trapenable();
	*ureg = realmoderegs;
}
