#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "msaturn.h"

enum {
	Isr = Saturn + 0x0400,
	Ipol = Saturn + 0x0404,
	Issr = Saturn + 0x0500,
	Ier = Saturn + 0x0504,
	Ipri = Saturn + 0x0600,
	Ithresh = Saturn + 0x0706,
#define Iar	Ithresh
};

enum{
	Syscfg = Saturn + 0x0100,
};

static uchar intprio[] = {
		Vecuart0,			// uart 0
		Vecunused,		// uart 1
		Vecunused,		// sint
		Vectimer0,		// timer 0
		Vecunused,		// timer 1
		Vecether,			// ethernet
		Vecunused,		// tea
		Vecunused,		// irq0	
		Vecunused,		// irq1	
		Vecunused,		// irq2	
		Vecunused,		// irq3	
		Vecunused,		// irq4	
		Vecunused,		// irq5	
		Vecunused,		// irq6	
		Vecunused,		// irq7	
		Vecunused,		// irq8	
};

void
intend(int)
{
}

void
hwintrinit(void)
{
	int i;

	*(ulong*)Ier=0;

	for(i=0; i<nelem(intprio)/2; i++)
		((uchar*)Ipri)[i] = (intprio[2*i]<<4)|intprio[2*i+1];
}

int
vectorenable(Vctl*v)
{
	int i;

	for(i=0; i<nelem(intprio); i++)
		if(v->irq==intprio[i]){
			*(ulong*)Ier |= 1<<(31-i);
			return v->irq;
		}
	print("intrenable: cannot enable intr %d\n", v->irq);
	return -1;
}

void
vectordisable(Vctl*v)
{
	int i;

	for(i=0; i<nelem(intprio); i++)
		if(v->irq==intprio[i]){
			*(ulong*)Ier &= ~(1<<(31-i));
			return;
		}
}

int
intvec(void)
{
	ushort iar;
	int i;

	iar = *(ushort*)Iar;		// push(prio) onto stack
	for(i=0; i<nelem(intprio); i++)
		if(iar==intprio[i])
			return iar;
		
	iprint("saturnint: no vector %d\n", iar);
	intack();
	return -1;
}

void
intack(void)
{
	*(ushort*)Ithresh = 0;	// pop(prio) stack
}

void
machinit(void)
{
	int rrate;
	ulong hid;
	extern char* plan9inistr;
	ulong l2cr;

	memset(m, 0, sizeof(*m));
	m->cputype = getpvr()>>16;
	m->imap = (Imap*)INTMEM;

	m->loopconst = 1096;

	rrate = (*(ushort*)Syscfg >> 6) & 3;
	switch(rrate){
	case 0:
		m->bushz = 66666666;
		break;
	case 1:
		m->bushz = 83333333;
		break;
	case 2:
		m->bushz = 100000000;
		break;
	case 3:
		m->bushz = 133333333;
		break;
	}

	if(getpll() == 0x80000000)
		m->cpuhz = 300000000;
	else
		m->cpuhz = 200000000;		/* 750FX? */
	m->cyclefreq = m->bushz / 4;

	active.machs = 1;
	active.exiting = 0;

	putmsr(getmsr() | MSR_ME);

	dcflush((void*)KZERO, 0x2000000);
	l2cr = getl2cr();
	putl2cr(l2cr|BIT(10));

	kfpinit();

	hid=gethid0();
	hid |= BIT(28)|BIT(26)|BIT(24);
	puthid0(hid);

	plan9inistr =
		"console=0\n"
		"ether0=type=saturn\n"
		"fs=135.104.9.42\n"
		"auth=135.104.9.7\n"
		"authdom=cs.bell-labs.com\n"
		"sys=ucu\n"
		"ntp=135.104.9.52\n";
}

void
sharedseginit(void)
{
}

void
trapinit(void)
{
	int i;

	for(i = 0x0; i < 0x2000; i += 0x100)
		sethvec(i, trapvec);

	dcflush(KADDR(0), 0x2000);
	icflush(KADDR(0), 0x2000);

	putmsr(getmsr() & ~MSR_IP);
}

void
reboot(void*, void*, ulong)
{
}
