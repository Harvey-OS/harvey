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

	ms *= 6230;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

/* initialize microsecond system timer */
void
timerinit(void)
{
	Systimer *st;

	st = (Systimer*)TIMER;
	st->low = 0xff;
	st->high = 0xff;
	st->csr = (1<<7)|(1<<6);
}

void
clock(Ureg *ur)
{
	Proc *p;
	int user, nrun = 0;

	user = (ur->sr&SUPER) == 0;
	if(user) {
		u->dbgreg = ur;
		u->p->pc = ur->pc;
	}

	m->ticks++;
	*(ulong*)VDMACSR |= (1<<20);
	kproftimer(ur->pc);
	p = m->proc;
	if(p){
		nrun = 1;
		p->pc = ur->pc;
		if(p->state==Running)
			p->time[p->insyscall]++;
	}
	nrun = (nrdy+nrun)*1000;
	MACHP(0)->load = (MACHP(0)->load*19+nrun)/20;
	checkalarms();
	kbdclock();
	sccclock();
	if((ur->sr&SPL(7))==0 && p && p->state==Running){
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if(user) {
			(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);	
			notify(ur);
		}
	}
}
