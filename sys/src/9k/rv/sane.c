/*
 * verify that various things work as expected:
 * data segment alignment, varargs, long dereferences, clock ticking.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

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

static void
signext(void)
{
	mpdigit ui, ui2, uiz;
	mpdigit *uip;
	uvlong uvl, vl;
	static mpdigit sui;

	uiz = 0;
	sui = ~uiz;
	uip = &sui;

	ui = sui;
	if (ui != sui)
		print("uint != uint\n");
//	print("ui (1) %#ux\n", ui);
	uvl = sui;
	if (uvl != sui)
		print("uvl != uint\n");
	if ((vlong)uvl < 0)
		print("uvl < 0 (1)\n");
//	print("uvl (1) %#llux\n", uvl);

	ui = *uip;
	if (ui != sui)
		print("uint != *uintp\n");
	uvl = *uip;
	if (uvl != sui)
		print("uvl != *uintp\n");
	if ((vlong)uvl < 0)
		print("uvl < 0 (2)\n");

	vl = *uip;
	if (vl != sui)
		print("vl != *uintp\n");
//	print("vl (1) %#llux\n", vl);
	if ((vlong)vl < 0)
		print("vl < 0\n");

	if ((ui>>1) != (vl>>1))
		print("ui>>1 != vl>>1\n");
	if((ui>>1) > ui)
		print("ui>>1 > ui\n");
	ui2 = ui<<8;
	if(ui2 >= ui)
		print("ui2 %#ux >= ui %#ux\n", ui2, ui);
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
	if (canlock(&testlock))
		panic("locks broken: acquired one twice");
	unlock(&testlock);

	varargck("", 1<<30, 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20));
	varargck("2", 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20));
	varargck("3", 2<<28, 0222, 3ll<<40 | 5<<20, 0111,
		(7ll<<40 | 12<<20), 0);
	negint();
	signext();
}

int
clocksanity(void)
{
	int i;
	ulong ticks;
	uvlong omtime;
	Clint *clint;

	assert(timebase >= 1000*1000);
	clint = clintaddr();
	clockoff(clint);
	ticks = m->ticks;
	m->clockintrsok = 1;
	setclinttmr(clint, VMASK(63));
	coherence();

	omtime = RDCLTIME(clint);
	clockenable();
	setclinttmr(clint, omtime + 20*(timebase/1000000));
	spllo();
	delay(10);
	for (i = 100; ticks == m->ticks && i > 0; i--)
		delay(10);

	splhi();
	if (ticks == m->ticks)
		iprint("clock not interrupting!\n");
	if (RDCLTIME(clint) <= omtime)
		iprint("clint clock is not advancing.\n");
	timerset(0);
	return ticks != m->ticks;
}
