#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	"ureg.h"

/*
 * Timer Configuration defines
 */
#define	TMR0_CONFIG		0x1		/* CPU 0 timer configuration */
#define	TMR1_CONFIG		0x2		/* CPU 1 timer configuration */
#define	TMR2_CONFIG		0x4		/* CPU 2 timer configuration */
#define	TMR3_CONFIG		0x8		/* CPU 3 timer configuration */
#define TMRALL_CONFIG		0xF		/* All configurations */

void
delay(int ms)
{
	int i;

	/* experimentally determined, for 51 (50Mhz, E$) */
	ms *= 24750;
	for(i=0; i<ms; i++)
		;
}

typedef struct Ctr Ctr;
struct Ctr
{
	ulong	limit;				/* r/w; resets counter; also user msw*/
	ulong	counter;			/* read only in counter mode; also user lsw */
	ulong	limit_noreset;		/* write only */
	ulong	control;			/* r/w; enables and disables user counter*/
};
Ctr	*ctr;

typedef struct Sysctr Sysctr;
struct Sysctr {
	ulong	limit;
	ulong	counter;
	ulong	limit_noreset;
	ulong	reserved;
	ulong	config;
};
Sysctr 	*sysctr;

void
clockinit(void)
{
	KMap *k;

	k = kmappas(IOSPACE, CLOCK, PTENOCACHE|PTEIO);
	ctr = (Ctr*)VA(k);
	ctr->limit = (CLOCKFREQ/HZ)<<10;

	k = kmappas(IOSPACE, SYSCLOCK, PTENOCACHE|PTEIO);
	sysctr = (Sysctr*)VA(k);
	sysctr->config = 0;	/* set to be system timer/counters */
}

void
clock(Ureg *ur)
{
	Proc *p;
	ulong nrun = 0;

	/* clear interrupt */
	ctr->limit = (CLOCKFREQ/HZ)<<10;

	m->ticks++;
	p = m->proc;
	if(p){
		nrun = 1;
		p->pc = ur->pc;
		if (p->state==Running)
			p->time[p->insyscall]++;
	}
	nrun = (nrdy+nrun)*1000;
	MACHP(0)->load = (MACHP(0)->load*19+nrun)/20;

	checkalarms();
	kbdclock();
	mouseclock();
	sccclock();
	kproftimer(ur->pc);
	if(m->fpunsafe)
		return;

	if((ur->psr&SPL(0xF))==0 && p && p->state==Running) {
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if((ur->psr&PSRPSUPER) == 0)
			*(ulong*)(USTKTOP-BY2WD) += TK2MS(1);
	}
}
