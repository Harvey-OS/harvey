#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"

#include "machine.h"

void
delay(int ms)
{
	extern Eeprom eeprom;

	ms *= 1000*ROUNDUP(eeprom.clock, 2)/2;
	while(ms-- > 0)
		;
}

void
clockintr(int ctlrno, Ureg *ur)
{
	Proc *p;
	int nrun = 0;

	USED(ctlrno);
	m->ticks++;
	/*
	 *  account for time
	 */
	p = m->proc;
	if(p){
		nrun = 1;
		p->pc = ur->pc;
		if(p->state == Running)
			p->time[p->insyscall]++;
	}
	nrun = (nrdy+nrun)*1000;
	MACHP(0)->load = (MACHP(0)->load*19+nrun)/20;
	checkalarms();
	eiaclock();
	kproftimer(ur->pc);
	if(u && p && p->state == Running){
		if(anyready()){
			if(p->hasspin)
				p->hasspin = 0;
			else
				sched();
		}
		if(ur->psw & PSW_X)
			*(ulong*)(USTKTOP-BY2WD) += TK2MS(1);
	}
}
