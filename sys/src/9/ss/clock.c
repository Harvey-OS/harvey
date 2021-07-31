#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	"ureg.h"

void
delay(int ms)
{
	int i;

	ms *= 6666;		/* experimentally determined */
	if(conf.ss2)
		ms *= 2;
	for(i=0; i<ms; i++)
		;
}

typedef struct Ctr Ctr;
struct Ctr
{
	ulong	ctr0;
	ulong	lim0;
	ulong	ctr1;
	ulong	lim1;
};
Ctr	*ctr;

void
clockinit(void)
{
	KMap *k;

	k = kmappa(CLOCK, PTENOCACHE|PTEIO);
	ctr = (Ctr*)k->va;
	ctr->lim1 = (CLOCKFREQ/HZ)<<10;
}

void
clock(Ureg *ur)
{
	Proc *p;
	ulong i, nrun = 0;

	i = ctr->lim1;	/* clear interrupt */
	USED(i);
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
