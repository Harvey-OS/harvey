#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

void
delay(int l)
{
	ulong i, j;

	j = m->delayloop;
	while(l-- > 0)
		for(i=0; i < j; i++)
			;
}

void
Xdelay(int us)
{
	int i;

	wbflush();
	us *= 12;	/* experimentally determined */
	for(i=0; i<us; i++)
		;
}

void
clockinit(void)
{
	long x;

	m->delayloop = m->speed*100;
	do {
		x = rdcount();
		delay(10);
		x = rdcount() - x;
	} while(x < 0);

	/*
	 *  fix count
	 */
	m->delayloop = (m->delayloop*m->speed*1000*10)/x;
	if(m->delayloop == 0)
		m->delayloop = 1;

	wrcompare(rdcount()+(m->speed*1000000)/HZ);
}

void
clock(Ureg *ur)
{
	Proc *p;
	int nrun = 0;

	wrcompare(rdcount()+(m->speed*1000000)/HZ);

	m->ticks++;
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
	if(u && (ur->status&IE) && p && p->state==Running){
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if(ur->status&KUSER)
			*(ulong*)(USTKTOP-BY2WD) += TK2MS(1);
	}
}
