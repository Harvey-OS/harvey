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

/*
 * AMD 82C54 timer
 *
 * ctr2 is clocked at 3.6864 MHz.
 * ctr2 output clocks ctr0 and ctr1.
 * ctr0 drives INTR2.  ctr1 drives INTR4.
 * To get 100Hz, 36864==9*4096=36*1024 so clock ctr2 every 1024 and ctr0 every 36.
 */

struct Timer{
	uchar	cnt0,
		junk0[3];
	uchar	cnt1,
		junk1[3];
	uchar	cnt2,
		junk2[3];
	uchar	ctl,
		junk3[3];
};


#define	TIME0	(36*MS2HZ/10)
#define	TIME1	0xFFFFFFFF	/* profiling disabled */
#define	TIME2	1024
#define	CTR(x)	((x)<<6)	/* which counter x */
#define	SET16	0x30		/* lsbyte then msbyte */
#define	MODE2	0x04		/* interval timer */



void
clockinit(void)
{
	Timer *t;
	int i;

	t = TIMERREG;
	t->ctl = CTR(2)|SET16|MODE2;
	t->cnt2 = TIME2&0xFF;
	t->cnt2 = (TIME2>>8)&0xFF;
	t->ctl = CTR(1)|SET16|MODE2;
	t->cnt1 = TIME1&0xFF;
	t->cnt1 = (TIME1>>8)&0xFF;
	t->ctl = CTR(0)|SET16|MODE2;
	t->cnt0 = TIME0;
	t->cnt0 = (TIME0>>8)&0xFF;
	i = *CLRTIM0;
	USED(i);
	i = *CLRTIM1;
	USED(i);
	m->ticks = 0;
}



void
clock(Ureg *ur)
{
	int i, nrun = 0;
	Proc *p;

	if(ur->cause & INTR2){
		i = *CLRTIM0;
		USED(i);
		m->ticks++;
		if(m->ticks&(1<<4))
			LEDON(LEDpulse);
		else
			LEDOFF(LEDpulse);
		if(m->proc)
			m->proc->pc = ur->pc;

		if(m->machno == 0){
			p = m->proc;
			if(p) {
				nrun++;
				p->time[p->insyscall]++;
			}
			for(i=1; i<conf.nmach; i++){
				if(active.machs & (1<<i)){
					p = MACHP(i)->proc;
					if(p) {
						p->time[p->insyscall]++;
						nrun++;
					}
				}
			}
			nrun = (nrdy+nrun)*1000;
			m->load = (m->load*19+nrun)/20;
		}
		duartslave();
		if((active.machs&(1<<m->machno)) == 0)
			return;
		if(active.exiting && active.machs&(1<<m->machno)){
			print("someone's exiting\n");
			exit(0);
		}
		checkalarms();
		kproftimer(ur->pc);
		if(u && (ur->status&IEP) && u->p->state==Running){
			if(anyready()) {
				if(u->p->hasspin)
					u->p->hasspin = 0;	/* just in case */
				else
					sched();
			}
			/* user profiling clock */
			if(ur->status & KUP)
				(*(ulong*)(USTKTOP-BY2WD)) += TK2MS(1);	
		}
		return;
	}
	if(ur->cause & INTR4){
		extern ulong start;

		i = *CLRTIM1;
		USED(i);
		return;
	}
}
