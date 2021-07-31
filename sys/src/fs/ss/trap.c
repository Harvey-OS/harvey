#include "all.h"
#include "io.h"
#include "mem.h"
#include "ureg.h"

extern	Label catch;
extern	void traplink(void);
extern	void syslink(void);

	long	ticks;
static	char	excbuf[64];	/* BUG: not reentrant! */

char *trapname[]={
	"reset",
	"instruction access exception",
	"illegal instruction",
	"privileged instruction",
	"fp: disabled",
	"window overflow",
	"window underflow",
	"unaligned address",
	"fp: exception",
	"data access exception",
	"tag overflow",
	"watchpoint detected",
};

char*
excname(ulong tbr)
{
	char xx[64];
	char *t;

	switch(tbr){
	case 8:
		panic("fptrap\n");
		return excbuf;
	case 36:
		return "trap: cp disabled";
	case 37:
		return "trap: unimplemented instruction";
	case 40:
		return "trap: cp exception";
	case 42:
		return "trap: divide by zero";
	case 128:
		return "syscall";
	case 129:
		return "breakpoint";
	}
	t = 0;
	if(tbr < sizeof trapname/sizeof(char*))
		t = trapname[tbr];
	if(t == 0){
		if(tbr >= 130)
			sprint(xx, "trap instruction %d", tbr-128);
		else if(17<=tbr && tbr<=31)
			sprint(xx, "interrupt level %d", tbr-16);
		else
			sprint(xx, "unknown trap %d", tbr);
		t = xx;
	}
	if(strncmp(t, "fp: ", 4) == 0)
		strcpy(excbuf, t);
	else
		sprint(excbuf, "trap: %s", t);
	return excbuf;
}

static void
intr(Ureg *ur, ulong tbr)
{
	switch(tbr-16){
	case 14:			/* counter 1 */
		clock(0, ur->pc);
		break;
	case 12:			/* keyboard and mouse */
		sccintr();
		break;
	case 5:				/* lance */
		lanceintr(0);
		break;
	case 3:				/* scsi */
		scsiintr(0);
		break;
	default:
		panic("interrupt: %s pc=0x%lux\n", excname(tbr), ur->pc);
	}
}

void
trap(Ureg *ur)
{
	ulong tbr;

	tbr = (ur->tbr&0xFFF)>>4;
	/*
	 * Hack to catch bootstrap fault during probe
	 */
	if(catch.pc)
		gotolabel(&catch);

	/* SS2 bug: flush cache line holding trap entry in table */
	if(mconf.ss2cachebug)
		putsysspace(CACHETAGS+((TRAPS+16*tbr)&(mconf.vacsize-1)), 0);
	if(tbr > 16)				/* interrupt */
		intr(ur, tbr);
	else{
		dumpregs(ur);
		panic("kernel trap: %s pc=0x%lux\n", excname(tbr), ur->pc);
	}
}

void
trapinit(void)
{
	int i;
	long t, a;

	a = ((ulong)traplink-TRAPS)>>2;
	a += 0x40000000;			/* CALL traplink(SB) */
	t = TRAPS;
	for(i=0; i<256; i++){
		/* ss2: must flush cache line for table */
		*(ulong*)(t+0) = a;		/* CALL traplink(SB) */
		*(ulong*)(t+4) = 0xa7480000;	/* MOVW PSR, R19 */
		a -= 16/4;
		t += 16;
	}
	/*
	 * Vector 128 goes directly to syslink
	 */
	t = TRAPS+128*16;
	a = ((ulong)syslink-t)>>2;
	a += 0x40000000;
	*(ulong*)t = a;			/* CALL syscall(SB) */
	*(ulong*)(t+4) = 0xa7480000;	/* MOVW PSR, R19 */
	puttbr(TRAPS);
	setpsr(getpsr()|PSRET|SPL(15));	/* enable traps, not interrupts */
}

void
dumpregs(Ureg *ur)
{
	int i;
	ulong *l, ser, sevar;

	if(u)
		print("registers for %d\n", u->pid);
	else
		print("registers for kernel\n");
	print("PSR=%ux PC=%lux TBR=%lux\n", ur->psr, ur->pc, ur->tbr);
	sevar = getsysspace(SEVAR);
	ser = getsysspace(SER);
	print("SER=%lux SEVAR=%lux\n", ser, sevar);
	l = &ur->r0;
	for(i=0; i<32; i+=2, l+=2)
		print("R%d\t%.8lux\tR%d\t%.8lux\n", i, l[0], i+1, l[1]);
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
				print("0x%lux?i\n", v);
		}
		if(hl)
		print("%ld stack used out of %ld\n",
			(ulong)(p->stack+MAXSTACK)-hl, MAXSTACK);
		print("\n");
	}
}

long
syscall(Ureg *ur)
{
	USED(ur);
	panic("system call\n");
	return 0;
}
