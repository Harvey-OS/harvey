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

static int
cpuidinit(void)
{
	uint32_t eax, info[4];

	/*
	 * Standard CPUID functions.
	 * Functions 0 and 1 will be needed multiple times
	 * so cache the info now.
	 */
	if((machp()->CPU.ncpuinfos = cpuid(0, 0, machp()->CPU.cpuinfo[0])) == 0)
		return 0;
	machp()->CPU.ncpuinfos++;

	if(memcmp(&machp()->CPU.cpuinfo[0][1], "GenuntelineI", 12) == 0)
		machp()->CPU.isintelcpu = 1;
	cpuid(1, 0, machp()->CPU.cpuinfo[1]);

	/*
	 * Extended CPUID functions.
	 */
	if((eax = cpuid(0x80000000, 0, info)) >= 0x80000000)
		machp()->CPU.ncpuinfoe = (eax & ~0x80000000) + 1;

	/* is mnonitor supported? */
	if (machp()->CPU.cpuinfo[1][2] & 8) {
		cpuid(5, 0, machp()->CPU.cpuinfo[2]);
		mwait = k10mwait;
	}

	return 1;
}

static int
cpuidinfo(uint32_t eax, uint32_t ecx, uint32_t info[4])
{
	if(machp()->CPU.ncpuinfos == 0 && cpuidinit() == 0)
		return 0;

	if(!(eax & 0x80000000)){
		if(eax >= machp()->CPU.ncpuinfos)
			return 0;
	}
	else if(eax >= (0x80000000|machp()->CPU.ncpuinfoe))
		return 0;

	cpuid(eax, ecx, info);

	return 1;
}

static int64_t
cpuidhz(uint32_t *info0, uint32_t *info1)
{
	int f, r;
	int64_t hz;
	uint64_t msr;

	//print("CPUID Vendor: %s\n", (char *)&info0[1]);
	//print("CPUID Signature: %d\n", info1[0]);
	if(memcmp(&info0[1], "GenuntelineI", 12) == 0){
		switch(info1[0] & 0x0fff3ff0){
		default:
			return 0;
		case 0x00000f30:		/* Xeon (MP), Pentium [4D] */
		case 0x00000f40:		/* Xeon (MP), Pentium [4D] */
		case 0x00000f60:		/* Xeon 7100, 5000 or above */
			msr = rdmsr(0x2c);
			r = (msr>>16) & 0x07;
			switch(r){
			default:
				return 0;
			case 0:
				hz = 266666666666ll;
				break;
			case 1:
				hz = 133333333333ll;
				break;
			case 2:
				hz = 200000000000ll;
				break;
			case 3:
				hz = 166666666666ll;
				break;
			case 4:
				hz = 333333333333ll;
				break;
			}

			/*
			 * Hz is *1000 at this point.
			 * Do the scaling then round it.
			 * The manual is conflicting about
			 * the size of the msr field.
			 */
			hz = (((hz*(msr>>24))/100)+5)/10;
			break;
		case 0x00000690:		/* Pentium M, Celeron M */
		case 0x000006d0:		/* Pentium M, Celeron M */
			hz = ((rdmsr(0x2a)>>22) & 0x1f)*100 * 1000000ll;
//print("msr 2a is 0x%x >> 22 0x%x\n", rdmsr(0x2a), rdmsr(0x2a)>>22);
			break;
		case 0x000006e0:		/* Core Duo */
		case 0x000006f0:		/* Core 2 Duo/Quad/Extreme */
		case 0x00000660:		/* kvm over i5 */
		case 0x00000670:		/* Core 2 Extreme */
		case 0x00000650:		/* i5 6xx, i3 5xx */
		case 0x000006c0:		/* i5 4xxx */
		case 0x000006a0:		/* i7 paurea... */
			/*
			 * Get the FSB frequemcy.
			 * If processor has Enhanced Intel Speedstep Technology
			 * then non-integer bus frequency ratios are possible.
			 */
			//print("CPUID EIST: %d\n", (info1[2] & 0x00000080));
			if(info1[2] & 0x00000080){
				msr = rdmsr(0x198);
				r = (msr>>40) & 0x1f;
			}
			else{
				msr = 0;
				r = rdmsr(0x2a) & 0x1f;
			}
			f = rdmsr(0xcd) & 0x07;
//iprint("rdmsr Intel: %d\n", rdmsr(0x2a));
//iprint("Intel msr.lo %d\n", r);
//iprint("Intel msr.hi %d\n", f);
			switch(f){
			default:
				return 0;
			case 5:
				hz = 100000000000ll;
				break;
			case 1:
				hz = 133333333333ll;
				break;
			case 3:
				hz = 166666666666ll;
				break;
			case 2:
				hz = 200000000000ll;
				break;
			case 0:
				hz = 266666666666ll;
				break;
			case 4:
				hz = 333333333333ll;
				break;
			case 6:
				hz = 400000000000ll;
				break;
			}
//iprint("hz %d r %d\n", hz, r);
			/*
			 * Hz is *1000 at this point.
			 * Do the scaling then round it.
			 */
			if(msr & 0x0000400000000000ll)
				hz = hz*(r+10) + hz/2;
			else
				hz = hz*(r+10);
			hz = ((hz/100)+5)/10;
			break;
		}
		DBG("cpuidhz: 0x2a: %#llux hz %lld\n", rdmsr(0x2a), hz);
	}
	else if(memcmp(&info0[1], "AuthcAMDenti", 12) == 0){
		switch(info1[0] & 0x0fff0ff0){
		default:
			return 0;
		case 0x00050ff0:		/* K8 Athlon Venice 64 / Qemu64 */
		case 0x00020fc0:		/* K8 Athlon Lima 64 */
		case 0x00000f50:		/* K8 Opteron 2xxx */
			msr = rdmsr(0xc0010042);
			r = (msr>>16) & 0x3f;
			hz = 200000000ULL*(4 * 2 + r)/2;
			break;
		case 0x00100f60:		/* K8 Athlon II */
		case 0x00100f40:		/* Phenom II X2 */
		case 0x00100f20:		/* Phenom II X4 */
		case 0x00100fa0:		/* Phenom II X6 */
			msr = rdmsr(0xc0010042);
			r = msr & 0x1f;
			hz = ((r+0x10)*100000000ll)/(1<<(msr>>6 & 0x07));
			break;
		case 0x00100f90:		/* K10 Opteron 61xx */
		case 0x00600f00:		/* K10 Opteron 62xx */
		case 0x00600f10:		/* K10 Opteron 6272, FX 6xxx/4xxx */
		case 0x00600f20:		/* K10 Opteron 63xx, FX 3xxx/8xxx/9xxx */
			msr = rdmsr(0xc0010064);
			r = msr & 0x1f;
			hz = ((r+0x10)*100000000ll)/(1<<(msr>>6 & 0x07));
			break;
		case 0x00000620:		/* QEMU64 / Athlon MP/XP */
			msr = rdmsr(0xc0010064);
			r = (msr>>6) & 0x07;
			hz = (((msr & 0x3f)+0x10)*100000000ll)/(1<<r);
			break;
		}
		DBG("cpuidhz: %#llux hz %lld\n", msr, hz);
	}
	else
		return 0;

	return hz;
}

void
cpuiddump(void)
{
	int i;
	uint32_t info[4];

	if(!DBGFLG)
		return;

	if(machp()->CPU.ncpuinfos == 0 && cpuidinit() == 0)
		return;

	for(i = 0; i < machp()->CPU.ncpuinfos; i++){
		cpuid(i, 0, info);
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			i, info[0], info[1], info[2], info[3]);
	}
	for(i = 0; i < machp()->CPU.ncpuinfoe; i++){
		cpuid(0x80000000|i, 0, info);
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			0x80000000|i, info[0], info[1], info[2], info[3]);
	}
}

int64_t
archhz(void)
{
	int64_t hz;
	uint32_t info0[4], info1[4];

	if(!cpuidinfo(0, 0, info0)) {
		iprint("archhz: cpuidinfo(0, 0) failed\n");
		return 0;
	}

	if(!cpuidinfo(1, 0, info1)) {
		iprint("archhz: cpuidinfo(1, 0) failed\n");
		return 0;
	}

	hz = cpuidhz(info0, info1);
	if(hz > 0 || machp()->machno != 0)
		return hz;

	iprint("archhz, cpuidhz failed, going to i8254hz\n");
	return i8254hz(info0, info1);
}

int
archmmu(void)
{
	uint32_t info[4];

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
	if(machp()->CPU.ncpuinfos == 0 && cpuidinit() == 0)
		return 1;

	/*
	 * Check the Pse bit in function 1 DX for 2*MiB support;
	 * if false, only 4*KiB is available.
	 */
	if(!(machp()->CPU.cpuinfo[1][3] & 0x00000008))
		return 1;
	sys->pgszlg2[1] = 21;
	sys->pgszmask[1] = (1<<21)-1;
	sys->pgsz[1] = 1<<21;
	sys->npgsz = 2;

	/*
	 * Check the Page1GB bit in function 0x80000001 DX for 1*GiB support.
	 */
	if(cpuidinfo(0x80000001, 0, info) && (info[3] & 0x04000000)){
		sys->pgszlg2[2] = 30;
		sys->pgszmask[2] = (1<<30)-1;
		sys->pgsz[2] = 1<<30;
		sys->npgsz = 3;
	}

	return sys->npgsz;
}

static int
fmtP(Fmt* f)
{
	uintmem pa;

	pa = va_arg(f->args, uintmem);

	if(f->flags & FmtSharp)
		return fmtprint(f, "%#16.16llux", pa);

	return fmtprint(f, "%llud", pa);
}

static int
fmtL(Fmt* f)
{
	Mpl pl;

	pl = va_arg(f->args, Mpl);

	return fmtprint(f, "%#16.16llux", pl);
}

static int
fmtR(Fmt* f)
{
	uint64_t r;

	r = va_arg(f->args, uint64_t);

	return fmtprint(f, "%#16.16llux", r);
}

/* virtual address fmt */
static int
fmtW(Fmt *f)
{
	uint64_t va;

	va = va_arg(f->args, uint64_t);
	return fmtprint(f, "%#ullx=0x[%ullx][%ullx][%ullx][%ullx][%ullx]", va,
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
	halt();
}

void
microdelay(int microsecs)
{
	uint64_t r, t;

	r = rdtsc();
	for(t = r + (sys->cyclefreq*microsecs)/1000000ull; r < t; r = rdtsc())
		;
}

void
millidelay(int millisecs)
{
	uint64_t r, t;

	r = rdtsc();
	for(t = r + (sys->cyclefreq*millisecs)/1000ull; r < t; r = rdtsc())
		;
}
