#include	"all.h"
#include	"mem.h"
#include	"ureg.h"
#include	"io.h"

/*
 * CAUSE register
 */
#define	EXCCODE(c)	((c>>2)&EXCMASK)
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

char *regname[]={
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

void
intr(Ureg *ur, ulong cause)
{
	uchar isr;
	static count = 0;

	lights(Lintr, 1);
	cause &= INTR5|INTR2|INTR1|INTR0;
	isr = ~(ISRce|ISRdsr|ISRdrs|ISRncr|*ISR);
	if(cause & INTR2){
		clock(cause, ur->pc);
		cause &= ~INTR2;
	}
	if(cause & INTR1){
		scsiintr(0);
		cause &= ~INTR1;
	}
	if(cause & INTR0){
		if(isr & ISRlance) {
			lanceintr(0);
			isr &= ~ISRlance;
		}
		if(isr & ISRscc) {
			sccintr();
			isr &= ~ISRscc;
		}
		if(isr == 0)
			cause &= ~INTR0;
	}
	if(cause)
		if(count < 10) {
			print("intr: cause %lux isr %lux\n", cause, isr);
			count++;
		}
	lights(Lintr, 0);
}

void
trap(Ureg *ur)
{
	int ecode;

	ecode = EXCCODE(ur->cause);
	switch(ecode){
	case CINT:
		m->intrp = 0;
		if(u && u->state==Running){
			intr(ur, ur->cause);
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
		panic("trap: %s\n", excname[ecode]);
	}
	splhi();
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
		print("%8.8s %.8lux  %8.8s %.8lux\n", regname[i], l[0], regname[i+1], l[1]);
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
