#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

enum
{
	Pindex=	0x198,
	Pdata=	0x199,
};

static Lock pmulock;
static int
pmuwrbit(int index, int bit, int pos)
{
	uchar x;

	lock(&pmulock);
	outb(Pindex, index);
	x = inb(Pdata);
	if(bit)
		x |= 1<<pos;
	else
		x &= ~(1<<pos);
	outb(Pindex, index);
	outb(Pdata, x);
	unlock(&pmulock);
	return 0;
}

static void
pmuwr(int index, int val)
{
	lock(&pmulock);
	outb(Pindex, index);
	outb(Pdata, val);
	unlock(&pmulock);
}

static uchar
pmurd(int index)
{
	uchar x;

	lock(&pmulock);
	outb(Pindex, index);
	x = inb(Pdata);
	unlock(&pmulock);
	return x;
}

static int
ncr3170serialpower(int onoff)
{
	return pmuwrbit(0x85, 1^onoff, 4);
}

static int
ncr3170snooze(ulong lastop, int wakeup)
{
	ulong diff;

	/* any system call or non-clock interrupt turns the clock back on */
	if(wakeup){
		/* set time from real time clock */
		diff = rtctime() - seconds();
		m->ticks += SEC2TK(diff);

		/* enable clock interrupts */
		int0mask &= ~(1<<(Clockvec&7));
		outb(Int0aux, int0mask);
		return 0;
	}

	if(TK2SEC(m->ticks - lastop) < 8)
		return 0;

	if((pmurd(0x80) & (1<<4)) == 0)
		return 0;

	/* mask out clock interrupts to allow the stuff above */
	int0mask |= 1<<(Clockvec&7);
	outb(Int0aux, int0mask);

	/* speed reduction after another 2 seconds */
	/* screen shutdown + 0MHz CPU after another minute */
	pmuwr(0x83, 0xf);
	pmuwr(0x84, 0x1);
	pmuwrbit(0x82, 1, 0);
	pmuwrbit(0x82, 1, 1);

	return 1;
}

PCArch ncr3170 =
{
	"NCRD.0",
	i8042reset,
	0,
	0,
	0,
	ncr3170serialpower,
	0,
	0,
	ncr3170snooze,
};
