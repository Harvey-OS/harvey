#include	"all.h"
#include	"mem.h"
#include	"ureg.h"
#include	"io.h"

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

int	int0mask = 0xff;	/* interrupts enabled for first 8259 */
int	int1mask = 0xff;	/* interrupts enabled for second 8259 */

/*
 *  trap/interrupt gates
 */
Segdesc ilt[256];
int badintr[16];

enum 
{
	Maxhandler=	128,		/* max number of interrupt handlers */
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
	Lock;
	Handler	*ivec[256];
	Handler	h[Maxhandler];
	int	free;
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

	lock(&halloc);
	if(halloc.free >= Maxhandler)
		panic("out of interrupt handlers");
	h = &halloc.h[halloc.free++];
	h->next = halloc.ivec[v];
	h->r = r;
	h->arg = arg;
	halloc.ivec[v] = h;
	unlock(&halloc);

	/*
	 *  enable corresponding interrupt in 8259
	 */
	if((v&~0x7) == Int0vec){
		int0mask &= ~(1<<(v&7));
		outb(Int0aux, int0mask);
	} else if((v&~0x7) == Int1vec){
		int1mask &= ~(1<<(v&7));
		outb(Int1aux, int1mask);
	}
}

/*
 *  set up the interrupt/trap gates
 */
void
trapinit(void)
{
	int i;

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
	sethvec(14, intr14, SEGIG, 0);	/* page fault, interrupts off */
	sethvec(15, intr15, SEGTG, 0);
	sethvec(16, intr16, SEGIG, 0);	/* math coprocessor, interrupts off */

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
	putidt(ilt, sizeof(ilt));

	/*
	 *  Set up the first 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int0ctl, 0x11);		/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int0aux, Int0vec);		/* ICW2 - interrupt vector offset */
	outb(Int0aux, 0x04);		/* ICW3 - have slave on level 2 */
	outb(Int0aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  Set up the second 8259 interrupt processor.
	 *  Make 8259 interrupts start at CPU vector Int0vec.
	 *  Set the 8259 as master with edge triggered
	 *  input with fully nested interrupts.
	 */
	outb(Int1ctl, 0x11);		/* ICW1 - edge triggered, master,
					   ICW4 will be sent */
	outb(Int1aux, Int1vec);		/* ICW2 - interrupt vector offset */
	outb(Int1aux, 0x02);		/* ICW3 - I am a slave on level 2 */
	outb(Int1aux, 0x01);		/* ICW4 - 8086 mode, not buffered */

	/*
	 *  pass #2 8259 interrupts to #1
	 */
	int0mask &= ~0x04;
	outb(Int0aux, int0mask);
}

char *excname[] = {
	[0]	"divide error",
	[1]	"debug exception",
	[2]	" nonmaskable interrupt",
	[3]	"breakpoint",
	[4]	"overflow",
	[5]	"bounds check",
	[6]	"invalid opcode",
	[7]	"coprocessor not available",
	[8]	"double fault",
	[9]	"9 (reserved)",
	[10]	"invalid TSS",
	[11]	"segment not present",
	[12]	"stack exception",
	[13]	"general protection violation",
	[14]	"page fault",
	[15]	"15 (reserved)",
	[16]	"coprocessor error",
};

Ureg lasttrap, *lastur;


/*
 *  All traps
 */
void
trap(Ureg *ur)
{
	int v;
	int c;
	Handler *h;

	v = ur->trap;

	/*
	 *  tell the 8259 that we're done with the
	 *  highest level interrupt (interrupts are still
	 *  off at this point)
	 */
	c = v&~0x7;
	if(c==Int0vec || c==Int1vec){
		outb(Int0ctl, EOI);
		if(c == Int1vec)
			outb(Int1ctl, EOI);
	}

	if(v>=256 || (h = halloc.ivec[v]) == 0){

		/* a processor or coprocessor error */
		if(v <= 16){
			dumpregs(ur);
			panic("%s pc=0x%lux\n", excname[v], ur->pc);
		}

		if(v >= Int0vec || v < Int0vec+16){
			/* an unknown interrupts */
			v -= Int0vec;
			if(badintr[v]++ == 0 || (badintr[v]%100000) == 0)
				print("unknown interrupt %d pc=0x%lux: total %d\n", v,
					ur->pc, badintr[v]);
		} else {
			/* unimplemented traps */
			print("illegal trap %d pc=0x%lux\n", v, ur->pc);
		}

		lasttrap = *ur;
		lastur = ur;
		return;
	}

	MACHP(0)->intrp = 0;
	/* there may be multiple handlers on one interrupt level */
	do {
		(*h->r)(ur, h->arg);
		h = h->next;
	} while(h);
	splhi();
	if(u && u->state == Running)
		sched();

	lasttrap = *ur;
	lastur = ur;
}

/*
 *  dump registers
 */
void
dumpregs2(Ureg *ur)
{
	print("FLAGS=%lux TRAP=%lux ECODE=%lux CS=%lux PC=%lux\n", ur->flags, ur->trap,
		ur->ecode, ur->cs&0xff, ur->pc);
	print("  AX %8.8lux  BX %8.8lux  CX %8.8lux  DX %8.8lux\n",
		ur->ax, ur->bx, ur->cx, ur->dx);
	print("  SI %8.8lux  DI %8.8lux  BP %8.8lux\n",
		ur->si, ur->di, ur->bp);
	print("  DS %4.4ux  ES %4.4ux  FS %4.4ux  GS %4.4ux\n",
		ur->ds&0xffff, ur->es&0xffff, ur->fs&0xffff, ur->gs&0xffff);
	print("  CR0 %8.8lux CR2 %8.8lux\n", getcr0(), getcr2());
}

void
dumpregs(Ureg *ur)
{
	dumpregs2(ur);
	print("  ur %lux\n", ur);
	dumpregs2(&lasttrap);
	print("  lastur %lux\n", lastur);
}

void
dumpstack(User *p)
{
	ulong i, l, v, hl;
	extern ulong etext;

	if(p == 0)
		return;
	hl = 0;
	print("stack trace of %d\n", p->pid);
	i = 0;
	for(l = (ulong)(p->stack+MAXSTACK)-4; l >= (ulong)(p->stack); l -= 4){
		v = *(ulong*)l;
		if(v)
			hl = l;
		if(KTZERO < v && v < (ulong)&etext){
			print("0x%8.8lux ", v);
			i++;
		}
		if(i == 8){
			i = 0;
			print("\n");
		}
	}
	if(hl)
		print("%ld stack used out of %ld\n",
			(ulong)(p->stack+MAXSTACK)-hl, MAXSTACK);
	print("\n");
}
