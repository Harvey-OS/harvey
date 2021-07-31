#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

/*
 *  8253 timer
 */
enum
{
	T0cntr=	0x40,		/* counter ports */
	T1cntr=	0x41,		/* ... */
	T2cntr=	0x42,		/* ... */
	Tmode=	0x43,		/* mode port */

	Load0square=	0x36,		/*  load counter 0 with 2 bytes,
					 *  output a square wave whose
					 *  period is the counter period
					 */
	Freq=		1193182,	/* Real clock frequency */
};

/*
 *  delay for l milliseconds
 */
void
delay(int l)
{
	int i;

	while(--l){
		for(i=0; i < 3000; i++) /* 3000 is correct for 33Mhz 486 */
			;
	}
}

void
clockinit(void)
{
	/*
	 *  set vector for clock interrupts
	 */
	setvec(Clockvec, clock);

	/*
	 *  make clock output a square wave with a 1/HZ period
	 */
	outb(Tmode, Load0square);
	outb(T0cntr, (Freq/HZ));	/* low byte */
	outb(T0cntr, (Freq/HZ)>>8);	/* high byte */
}

void
clock(Ureg *ur)
{
	Proc *p;

	m->ticks++;

	checkalarms();

	p = m->proc;
	if(p){
		p->pc = ur->pc;
		if (p->state==Running)
			p->time[p->insyscall]++;
	}
}
