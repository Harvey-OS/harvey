/*
 * verify that various things work as expected:
 * data segment alignment, varargs, long dereferences, clock ticking.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "riscv.h"

/* verify varargs machinery */
static void
varargck(char *fmt, ...)
{
	uint i, j;
	uchar c, c2;
	uvlong vl, vl2;
	va_list list;

	va_start(list, fmt);
	i = 0;
	if (fmt[0] == '\0')
		i = va_arg(list, uint);
	j = va_arg(list, uint);
	c  = va_arg(list, uchar);
	vl = va_arg(list, uvlong);
	c2  = va_arg(list, uchar);
	vl2 = va_arg(list, uvlong);
	va_end(list);

	if (fmt[0] == '\0')
		if (i != (1<<30))
			iprint("varargck %s int i vararg botch\n", fmt);
	if (j != (2<<28))
		iprint("varargck %s int j vararg botch\n", fmt);
	if (c != 0222)
		iprint("varargck %s char c vararg botch\n", fmt);
	if (vl != (3ll<<40 | 5<<20))
		iprint("varargck %s vlong vl vararg botch\n", fmt);
	if (c2 != 0111)
		iprint("varargck %s char c2 vararg botch\n", fmt);
	if (vl2 != (7ll<<40 | 12<<20))
		iprint("varargck %s vlong vl2 vararg botch\n", fmt);
}

static long
negfunc(void)
{
	long l = -1;

	return l;
}

static void
negint(void)
{
	long l = -1;
	long *lp;

	lp = &l;
	if (*lp >= 0)
		print("*lp >= 0!\n");
	if (negfunc() >= 0)
		print("negfunc() >= 0!\n");
}

static vlong zvl;		/* in bss */

void
sanity(void)
{
	static Lock testlock;
	static ulong align = 123456;

	if (align != 123456)
		panic("mis-aligned data segment; expected %#x saw %#lux",
			123456, align);
	if (zvl != 0)
		panic("mis-initialized bss; expected 0 saw %#llux", zvl);

	assert(sys != 0);
	if (KTZERO != (uintptr)_main)
		panic("KTZERO %#p != _main %#p", KTZERO, _main);
	if ((uintptr)sys < KSEG0)
		panic("sys %#p < KSEG0 %#p", sys, KSEG0);

	lock(&testlock);
	unlock(&testlock);

	varargck("", 1<<30, 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20));
	varargck("2", 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20));
	varargck("3", 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20), 0);
	negint();
}

static int
chkwfi(Clint *clint, int lo, int pow)
{
	uvlong omtime, delticks, slopticks;
	Mreg ie;

	if (1)				// TODO
		return 0;
	assert(sys->clintsperµs >= 1);
	delticks = (1ull << pow) * sys->clintsperµs;
	slopticks = pow <= 10? 1000 * sys->clintsperµs: delticks;

	coherence();
	omtime = clint->mtime;
	clrstip();
	setclinttmr(clint, omtime + delticks);
	coherence();

	if (lo)
		spllo();
	/* arrange that any interrupt will just resume WFI */
	ie = getsie();
	putsie(ie | (Ssie|Stie|Seie));
	halt();
	putsie(ie);
	splhi();

	setclinttmr(clint, VMASK(63));
	coherence();
	clrstip();
	if (clint->mtime - omtime > delticks + slopticks) {
		iprint("failed: clint ticks %,lld -> %,lld for %lld ticks\n",
			omtime, clint->mtime, delticks);
		idlepause = 1;		/* can't trust wfi */
		return -1;
	}
	return 0;
}

int
clocksanity(void)
{
	int i, rv;
	uvlong omtime;
	Clint *clint;

	rv = 0;
	m->clockintrsok = 0;
	splhi();
	assert(timebase >= 1000*1000);
	clint = clintaddr();
	clockoff(clint);
	if (m->machno == 0)
		clint->mtime = 0;	/* try to reset; may not take */
	setclinttmr(clint, VMASK(63));
	coherence();
	clrstip();

	if (m->machno != 0)		/* assume cpus are identical */
		goto bail;

	omtime = clint->mtime;
	setclinttmr(clint, omtime + 20*sys->clintsperµs);
	delay(1);
	coherence();
	for (i = 1000; clint->mtime == omtime && i > 0; i--) {
		delay(1);
		coherence();
	}

	rv = -1;
	if (clint->mtime == omtime)
		panic("clint clock is not advancing");
	if (clint->mtime < omtime)
		panic("clint clock is counting backwards");

	/* clock is okay, but does wfi work per the isa spec.? */
	iprint("testing wfi splhi...");
	/* don't trigger hzclock */
	for (i = 0; i < 16; i++)
		if (chkwfi(clint, 0, i) < 0)
			goto bail;
	iprint("spllo...");
	for (i = 0; i < 16; i++)
		if (chkwfi(clint, 1, i) < 0)
			goto bail;
	iprint("wfi ok.\n");
	rv = 0;
bail:
	setclinttmr(clint, VMASK(63));
	coherence();
	clrstip();
	clockenable();
	m->clockintrsok = 1;
	return rv;
}
