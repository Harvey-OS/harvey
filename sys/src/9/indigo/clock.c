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
Xdelay(int us)
{
	int i;

	wbflush();
	us *= 7;	/* experimentally determined */
	for(i=0; i<us; i++)
		;
}

/* #define	PROFILING /**/
#ifdef PROFILING
#undef TIME1
#define	TIME1	211		/* profiling clock; prime; about 10ms per tick */
#define	NPROF	50000
ulong	profcnt[MAXMACH*NPROF];
#endif

#define C2FREQ	(100000L)
#define C1FREQ	(200L)
#define C0FREQ	(HZ)

void
clockinit(void)
{
	Pt_clock *pt = IO(Pt_clock, PT_CLOCK);
	int s = splhi();

	pt->cw = PTCW_SC(2)|PTCW_16B|MODE_RG;
	pt->c2 = MASTER_FREQ/C2FREQ;
	Xdelay(4);
	pt->c2 = (MASTER_FREQ/C2FREQ)>>8;

	pt->cw = PTCW_SC(1)|PTCW_16B|MODE_RG;
	pt->c1 = C2FREQ/C1FREQ;
	Xdelay(4);
	pt->c1 = (C2FREQ/C1FREQ)>>8;

	pt->cw = PTCW_SC(0)|PTCW_16B|MODE_RG;
	pt->c0 = C2FREQ/C0FREQ;
	Xdelay(4);
	pt->c0 = (C2FREQ/C0FREQ)>>8;

	splx(s);
}

void
clock(Ureg *ur)
{
	Proc *p;
	int nrun = 0;
	static int netlight;

	m->ticks++;
	/*
	 *  network light
	 */
	if((m->ticks & 0xf) == 0){
		if(netlight == 0){
			if(m->nettime){
				netlight = 1;
				lightbits(0, 4);/**/
			}
		}else{
			if(m->ticks - m->nettime > HZ/3){
				lightbits(4, 0);/**/
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
	mouseclock();
	sccclock();
	kproftimer(ur->pc);
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
