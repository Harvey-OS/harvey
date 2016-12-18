/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#undef DBG
#define DBG iprint

void
cpuiddump(void)
{
	print("riscv\n");
}

int64_t
archhz(void)
{
	return 1000*1000*1000*2ULL;
}

int
archmmu(void)
{

	/*
	 * Should the check for machp()->machno != 0 be here
	 * or in the caller (mmuinit)?
	 *
	 * To do here:
	 * check and enable Pse;
	 * Pge; Nxe.
	 */

	/*
	 * How many page sizes are there?
	 * Always have 4*KiB, but need to check
	 * configured correctly.
	 */
	assert(PGSZ == 4*KiB);

	sys->pgszlg2[0] = 12;
	sys->pgszmask[0] = (1<<12)-1;
	sys->pgsz[0] = 1<<12;
	sys->npgsz = 1;

	sys->pgszlg2[1] = 21;
	sys->pgszmask[1] = (1<<21)-1;
	sys->pgsz[1] = 1<<21;
	sys->npgsz = 2;

		sys->pgszlg2[2] = 30;
		sys->pgszmask[2] = (1<<30)-1;
		sys->pgsz[2] = 1<<30;
		sys->npgsz = 3;

	return sys->npgsz;
}

static int
fmtP(Fmt* f)
{
	uintmem pa;

	pa = va_arg(f->args, uintmem);

	if(f->flags & FmtSharp)
		return fmtprint(f, "%#16.16llx", pa);

	return fmtprint(f, "%llu", pa);
}

static int
fmtL(Fmt* f)
{
	Mpl pl;

	pl = va_arg(f->args, Mpl);

	return fmtprint(f, "%#16.16llx", pl);
}

static int
fmtR(Fmt* f)
{
	uint64_t r;

	r = va_arg(f->args, uint64_t);

	return fmtprint(f, "%#16.16llx", r);
}

/* virtual address fmt */
static int
fmtW(Fmt *f)
{
	uint64_t va;

	va = va_arg(f->args, uint64_t);
	return fmtprint(f, "%#llx=0x[%llx][%llx][%llx][%llx][%llx]", va,
		PTLX(va, 3), PTLX(va, 2), PTLX(va, 1), PTLX(va, 0),
		va & ((1<<PGSHFT)-1));

}

void
archfmtinstall(void)
{
	/*
	 * Architecture-specific formatting. Not as neat as they
	 * could be (e.g. there's no defined type for a 'register':
	 *	L - Mpl, mach priority level
	 *	P - uintmem, physical address
	 *	R - register
	 * With a little effort these routines could be written
	 * in a fairly architecturally-independent manner, relying
	 * on the compiler to optimise-away impossible conditions,
	 * and/or by exploiting the innards of the fmt library.
	 */
	fmtinstall('P', fmtP);
	fmtinstall('L', fmtL);
	fmtinstall('R', fmtR);
	fmtinstall('W', fmtW);
}

void
archidle(void)
{
panic("archidle"); //	halt();
}

void
microdelay(int microsecs)
{
print("microdelay\n");
/*
	uint64_t r, t;

	r = rdtsc();
	for(t = r + (sys->cyclefreq*microsecs)/1000000ull; r < t; r = rdtsc())
		;
 */
}

void
millidelay(int millisecs)
{
print("millidelay\n");
/*
	uint64_t r, t;

	r = rdtsc();
	for(t = r + (sys->cyclefreq*millisecs)/1000ull; r < t; r = rdtsc())
		;
 */
}
