#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"mem.h"
#include	"ureg.h"
#include	"io.h"

#define	LSYS	0x01
#define	LUSER	0x02
/*
 * CAUSE register
 */

#define	EXCCODE(c)	((c>>2)&(NEXCODE-1))
#define	FPEXC		16

char *excname[NEXCODE] =
{
	"external interrupt",
	"TLB modification",
	"TLB miss (load or fetch)",
	"TLB miss (store)",
	"address error (load or fetch)",
	"address error (store)",
	"bus error (fetch)",
	"bus error (data load or store)",
	"system call",
	"breakpoint",
	"reserved instruction",
	"coprocessor unusable",
	"arithmetic overflow",
	"trap exception",
	"virtual coherency exception instruction",
	"machine check",
	/* the following is made up */
	"floating point exception"		/* FPEXC */
};

char *regname[]={
	"STATUS",	"PC",
	"SP",		"CAUSE",
	"BADADDR",	"ERROR",
	"HI",		"LO",
	"R31",		"R30",
	"R28",		"R27",
	"R26",		"R25",
	"R24",		"R23",
	"R22",		"R21",
	"R20",		"R19",
	"R18",		"R17",
	"R16",		"R15",
	"R14",		"R13",
	"R12",		"R11",
	"R10",		"R9",
	"R8",		"R7",
	"R6",		"R5",
	"R4",		"R3",
	"R2",		"R1",
};

long	ticks;
ulong	probepc;

/*
 * we can get bus-errors from IOC deadlock.
 * we need to retry the operation.
 */
static int
retryio(Ureg *ur)
{
	static unsigned count, badvaddr, pc;
	int i;

	/*
	 * should check badvaddr is in IOC range
	 */
	if (ur->badvaddr != badvaddr) {
		badvaddr = ur->badvaddr;
		count = 0;
	}
	if (ur->pc != pc) {
		pc = ur->pc;
		count = 0;
	}
	if (count >= 2048)
		return 0;
	for (i = 0; i < 10; i++)
		;
	return 1;
}

void
intr(Ureg *ur, ulong cause)
{
	ulong mask, bit, intr;

	lights(Lintr, 1);
	cause &= INTR2|INTR1|INTR0;
	if(cause & (INTR0)) {
		mask = SBCREG->intvecset;
		for(bit = 1, intr = 0; intr < 32 && mask; intr++, bit <<= 1) {
			if((bit & mask) == 0)
				continue;
			switch(intr) {

			case 0:
				clock(TIMERIE, ur->pc);
				break;

			case 3:
				duartintr();
				break;

			case 4:				/* VME1 */
				vmeintr(1);
				break;

			case 5:				/* VME0 */
				vmeintr(0);
				break;

			case 2:				/* ioa error */
				ioaintr();
				break;

			case 1:				/* mem error */
			case 6:				/* VME1 LOP */
			case 7:				/* VME0 LOP */
			case 8:				/* VME3 */
			case 9:				/* VME2 */
			case 10:			/* VME3 LOP */
			case 11:			/* VME2 LOP */
			case 12:			/* VME5 */
			case 13:			/* VME4 */
			case 14:			/* VME5 LOP */
			case 15:			/* VME4 LOP */
				/*FALLTHROUGH*/
			default:
				print("unexpected external interrupt mask=0x%lux\n", mask);
				intrclr(bit);
				dumpregs(ur);
				for(;;);
				panic("");
			}
			intrclr(bit);
			mask &= ~bit;
		}
		cause &= ~INTR0;
	}
	if(cause & INTR1) {
		panic("fp interrupt\n");
		cause &= ~INTR1;
	}
	if(cause)
		print("unexpected interrupt %lux %lux\n", u, cause);
	lights(Lintr, 0);
}

void
trap(Ureg *ur)
{
	int ecode;

	ecode = EXCCODE(ur->cause);
	switch(ecode) {
	case CINT:
		m->intrp = 0;
		if(u && u->state==Running){
/*
			m->intr = intr;
			m->cause = ur->cause;
			m->ureg = ur;
			if(ur->cause & INTR0)
				m->intrp = u;
*/
			intr(ur, ur->cause);
			sched();
		} else
			intr(ur, ur->cause);
		break;

	case CBUSD:
		if (probepc) {
			ur->pc = probepc;
			return;
		}
		if (retryio(ur))
			return;
		/*FALLTHROUGH*/

	default:
		/*
		 * This isn't good enough;
		 * can still deadlock because we may hold print's locks
		 * in this processor.
		 */
		lights(Lpanic, 1);	/* just to make sure */
		dumpregs(ur);
		dumpstack(u);
		print("*pc = %lux\n", *(ulong*)ur->pc);
		print("sbc err = %lux\n", SBCREG->errreg);
		panic("trap: %s\n", excname[ecode]);
	}
	splhi();
}

long
syscall(Ureg *u)
{

	USED(u);
	print("syscall");
	return -1;
}

void
dumpregs(Ureg *ur)
{
	int i;
	ulong *l;

	if(u)
		print("registers for %d\n", u->pid);
	else
		print("registers for kernel\n");
	l = &ur->status;
	for(i=0; i<sizeof regname/sizeof(char*); i+=2, l+=2)
		print("%s\t%.8lux\t%s\t%.8lux\n", regname[i], l[0], regname[i+1], l[1]);
}

void
dumpstack(User *p)
{
	ulong l, v, hl;
	extern ulong etext;

	hl = 0;
	if(p) {
		print("stack trace of %d\n", p->pid);
		for(l=(ulong)(p->stack+MAXSTACK)-4; l>=(ulong)(p->stack); l-=4) {
			v = *(ulong*)l;
			if(v)
				hl = l;
			if(KTZERO < v && v < (ulong)&etext)
				print("0x%lux?i\n", v-8);
		}
		if(hl)
		print("%ld stack used out of %ld\n",
			(ulong)(p->stack+MAXSTACK)-hl, MAXSTACK);
		print("\n");
	}
}
