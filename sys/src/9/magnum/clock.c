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

	ms *= 7000;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

void
Xdelay(int ms)
{
	int i;

	ms *= 7;	/* experimentally determined */
	for(i=0; i<ms; i++)
		;
}

/* #define	PROFILING /**/
#ifdef PROFILING
#undef TIME1
#define	TIME1	211		/* profiling clock; prime; about 10ms per tick */
#define	NPROF	50000
ulong	profcnt[MAXMACH*NPROF];
#endif

#define COUNT	((6250000L*MS2HZ)/(1000L))		/* counts per HZ */

void
clockinit(void)
{
	*TCOUNT = 0;
	*TBREAK = COUNT;
	wbflush();
}

void
clock(Ureg *ur)
{
	ulong dt;
	Proc *p;
	int nrun = 0;
	static int netlight;

	/*
	 *  reset clock
	 */
	do{
		*TBREAK += COUNT; /* just let it overflow and wrap around */
		wbflush();
		m->ticks++;
		dt = *TBREAK - *TCOUNT;
	}while(dt == 0 || dt > COUNT);

	/*
	 *  network light
	 */
	if((m->ticks & 0xf) == 0){
		if(netlight == 0){
			if(m->nettime){
				netlight = 1;
				lights(1);
			}
		} else {
			if(m->ticks - m->nettime > HZ/3){
				lights(0);
				netlight = 0;
				m->nettime = 0;
			}
		}
	}

	/*
	 *  account for time
	 */
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
	sccclock();
	kproftimer(ur->pc);
	mouseclock();
	if(u && (ur->status&IEP) && p && p->state==Running){
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if(ur->status&KUP)
			*(ulong*)(USTKTOP-BY2WD) += TK2MS(1);
	}
}
