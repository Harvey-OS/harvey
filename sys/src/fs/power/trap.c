#include	"all.h"
#include	"mem.h"
#include	"ureg.h"
#include	"io.h"

#define	LSYS	0x01
#define	LUSER	0x02
/*
 * CAUSE register
 */

#define	EXCCODE(c)	((c>>2)&0x0F)
#define	FPEXC		16

char *excname[] =
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
	"undefined 13",
	"undefined 14",
	"undefined 15",
	/* the following is made up */
	"floating point exception"		/* FPEXC */
};

char *regname[] =
{
	"STATUS",	"PC",
	"SP",		"CAUSE",
	"BADADDR",	"TLBVIRT",
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

static
void
clrbuserr(Ureg *ur, int which)
{
	ulong be1;


	be1 = *MPBERR1;
	print("MP bus error %svalid (%d) %lux %lux %lux\n",
		(be1 & (1<<8))? "not ": "",
		which, *MPBERR0, be1, ur->pc);
	*MPBERR0 = 0;
}

void
intr(Ureg *ur, ulong cause)
{
	int i, any;
	uchar pend, xxx;
	long v;
	ulong ima;

	lights(Lintr, 1);

	cause &= INTR5|INTR4|INTR3|INTR2|INTR1;
	if(cause & INTR1) {
		duartintr();
		cause &= ~INTR1;
	}

	while(cause & INTR5) {
		any = 0;
		if(!(*MPBERR1 & (1<<8))) {
			if(probeflag) {
				probeflag = 0;
				ur->pc += 4;
				*MPBERR0 = 0;
			} else
				clrbuserr(ur, 0);
			i = *SBEADDR;
			USED(i);
			any = 1;
		}

		/*
		 *  directions from IO2 manual
		 *  1. clear all IO2 masks
		 */
		*IO2CLRMASK = 0xff000000;

		/*
		 *  2. wait for interrupt in progress
		 */
		while(!(*INTPENDREG & (1<<5)))
			;

		/*
		 *  3. read pending interrupts
		 */
		pend = SBCCREG->fintpending;

		/*
		 *  4. clear pending register
		 */
		i = SBCCREG->flevel;
		USED(i);

		/*
		 *  4a. attempt to fix problem
		 */
		if(!(*INTPENDREG & (1<<5)))
			print("pause again\n");
		while(!(*INTPENDREG & (1<<5)))
			;
		xxx = SBCCREG->fintpending;
		if(xxx){
			print("new pend %ux\n", xxx);
			i = SBCCREG->flevel;
			USED(i);
		}

		/*
		 *  5a. process lance, scsi
		 */
		if(pend & 1) {
			v = INTVECREG->i[0].vec;
			if(!(v & (1<<12))){
				clrbuserr(ur, 1);
				any = 1;
			}
			if(ioid < IO3R1){
				if(!(v & 7))
					any = 1;
				if(!(v & (1<<2)))
					lanceintr(0);
				if(!(v & (1<<1)))
					lanceparity();
				if(!(v & (1<<0)))
					print("SCSI interrupt\n");
			}else{
				if(v & 7)
					any = 1;
				if(v & (1<<2))
					lanceintr(0);
				if(v & (1<<1))
					scsiintr(1);
				if(v & (1<<0))
					scsiintr(0);
			}
		}

		/*
		 *  5b. process vme
		 *  i bet i can guess your level
		 */
		for(i=1; pend>>=1; i++){
			if(pend & 1) {
				v = INTVECREG->i[i].vec;
				if(!(v & (1<<12)))
					clrbuserr(ur, 2);
				/* No registered handler */
				if(vmeintr(v & 0xFF) == 0) {
					ima = *IO2CLRMASK;
					print("VME: noack vec=%d imask=0x%lux\n",
						v & 0xff, ima);
				}
				any = 1;
			}
		}
		/*
		 *  6. re-enable interrupts
		 */
		*IO2SETMASK = 0xff000000;
		if(any == 0)
			cause &= ~INTR5;
	}
	if(cause & (INTR2|INTR4)) {
		clock(cause, ur->pc);
		cause &= ~(INTR2|INTR4);
	}
	if(cause)
		print("cause %lux %lux\n", u, cause);
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
			m->intr = intr;
			m->cause = ur->cause;
			m->ureg = ur;
			if(ur->cause & INTR2)
				m->intrp = u;
			sched();
		} else
			intr(ur, ur->cause);
		break;

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
		panic("trap");
	}
	splhi();
}

long
syscall(Ureg *u)
{

	USED(u);
	print("syscall");
	return 0;
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
	for(i=0; i<nelem(regname); i+=2, l+=2)
		print("%s\t%.8lux\t%s\t%.8lux\n",
			regname[i], l[0], regname[i+1], l[1]);
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

/*
 * serious troubles with this routine
 * different memory areas require
 * different treatment. also this
 * cant be multi-programmed since
 * probeflag is not in m->.
 */
#ifdef xx
int
probe(void *x, int size)
{
	int i, s;

	s = spllo();
	for(i=0; i<100; i++)
		;
	probeflag = 1;
	switch(size) {
	default:
		i = *(long*)((ulong)x & ~3);
		break;
	case 2:
		i = *(short*)((ulong)x & ~1);
		break;
	case 1:
		i = *(char*)x;
		break;
	}
	splx(s);
	USED(i);
	if(probeflag) {
		probeflag = 0;
		return 0;
	}
	return 1;
}
#endif
