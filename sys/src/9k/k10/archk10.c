#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

static int
portwaitfor(int *vp, int val)
{
	int i;

	/*
	 * How many times round this loop?
	 */
	for(i = 0; *vp == val && i < 10000; i++)
		;

	return *vp;
}

int (*waitfor)(int*, int) = portwaitfor;

extern int k10waitfor(int*, int);

static int
cpuidinit(void)
{
	u32int eax, info[4];

	/*
	 * Standard CPUID functions.
	 * Functions 0 and 1 will be needed multiple times
	 * so cache the info now.
	 */
	if((m->ncpuinfos = cpuid(0, 0, m->cpuinfo[0])) == 0)
		return 0;
	m->ncpuinfos++;

	if(memcmp(&m->cpuinfo[0][1], "GenuntelineI", 12) == 0)
		m->isintelcpu = 1;
	cpuid(1, 0, m->cpuinfo[1]);

	/*
	 * Extended CPUID functions.
	 */
	if((eax = cpuid(0x80000000, 0, info)) >= 0x80000000)
		m->ncpuinfoe = (eax & ~0x80000000) + 1;

	/*
	 * Is MONITOR/MWAIT supported?
	 */
	if(m->cpuinfo[1][2] & 8){
		/*
		 * Will be interested in parameters,
		 * extensions, and hints later; they can be retrieved
		 * with standard CPUID function 5.
		 */
		waitfor = k10waitfor;
	}

	return 1;
}

static int
cpuidinfo(u32int eax, u32int ecx, u32int info[4])
{
	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return 0;

	if(!(eax & 0x80000000)){
		if(eax >= m->ncpuinfos)
			return 0;
	}
	else if(eax >= (0x80000000|m->ncpuinfoe))
		return 0;

	cpuid(eax, ecx, info);

	return 1;
}

static vlong
cpuidhz(u32int info[2][4])
{
	int f, r;
	vlong hz;
	u64int msr;

	if(memcmp(&info[0][1], "GenuntelineI", 12) == 0){
		switch(info[1][0] & 0x0fff3ff0){
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
			break;
		case 0x000006e0:		/* Core Duo */
		case 0x000006f0:		/* Core 2 Duo/Quad/Extreme */
		case 0x00010670:		/* Core 2 Extreme */
			/*
			 * Get the FSB frequemcy.
			 * If processor has Enhanced Intel Speedstep Technology
			 * then non-integer bus frequency ratios are possible.
			 */
			if(info[1][2] & 0x00000080){
				msr = rdmsr(0x198);
				r = (msr>>40) & 0x1f;
			}
			else{
				msr = 0;
				r = rdmsr(0x2a) & 0x1f;
			}
			f = rdmsr(0xcd) & 0x07;
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

			/*
			 * Hz is *1000 at this point.
			 * Do the scaling then round it.
			 */
			if(msr & 0x0000400000000000ll)
				hz = hz*r + hz/2;
			else
				hz = hz*r;
			hz = ((hz/100)+5)/10;
			break;
		}
		DBG("cpuidhz: 0x2a: %#llux hz %lld\n", rdmsr(0x2a), hz);
	}
	else if(memcmp(&info[0][1], "AuthcAMDenti", 12) == 0){
		switch(info[1][0] & 0x0fff0ff0){
		default:
			return 0;
		case 0x00000f50:		/* K8 */
			msr = rdmsr(0xc0010042);
			if(msr == 0)
				return 0;
			hz = (800 + 200*((msr>>1) & 0x1f)) * 1000000ll;
			break;
		case 0x00100f90:		/* K10 */
		case 0x00000620:		/* QEMU64 */
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
	u32int info[4];

	if(!DBGFLG)
		return;

	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return;

	for(i = 0; i < m->ncpuinfos; i++){
		cpuid(i, 0, info);
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			i, info[0], info[1], info[2], info[3]);
	}
	for(i = 0; i < m->ncpuinfoe; i++){
		cpuid(0x80000000|i, 0, info);
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			0x80000000|i, info[0], info[1], info[2], info[3]);
	}
}

vlong
archhz(void)
{
	vlong hz;
	u32int info[2][4];

	if(DBGFLG && m->machno == 0)
		cpuiddump();
	if(!cpuidinfo(0, 0, info[0]) || !cpuidinfo(1, 0, info[1]))
		return 0;

	hz = cpuidhz(info);
	if(hz != 0)
		return hz;
	else if(m->machno != 0)
		return sys->machptr[0]->cpuhz;

	return i8254hz(info);
}

int
archmmu(void)
{
	u32int info[4];

	/*
	 * Should the check for m->machno != 0 be here
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

	m->pgszlg2[0] = 12;
	m->pgszmask[0] = (1<<12)-1;
	m->npgsz = 1;
	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return 1;

	/*
	 * Check the Pse bit in function 1 DX for 2*MiB support;
	 * if false, only 4*KiB is available.
	 */
	if(!(m->cpuinfo[1][3] & 0x00000008))
		return 1;
	m->pgszlg2[1] = 21;
	m->pgszmask[1] = (1<<21)-1;
	m->npgsz = 2;

	/*
	 * Check the Page1GB bit in function 0x80000001 DX for 1*GiB support.
	 */
	if(cpuidinfo(0x80000001, 0, info) && (info[3] & 0x04000000)){
		m->pgszlg2[2] = 30;
		m->pgszmask[2] = (1<<30)-1;
		m->npgsz = 3;
	}

	return m->npgsz;
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
	u64int r;

	r = va_arg(f->args, u64int);

	return fmtprint(f, "%#16.16llux", r);
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
}

void
microdelay(int microsecs)
{
	u64int r, t;

	r = rdtsc();
	for(t = r + m->cpumhz*microsecs; r < t; r = rdtsc())
		pause();
}

void
millidelay(int millisecs)
{
	u64int r, t;

	r = rdtsc();
	for(t = r + m->cpumhz*1000ull*millisecs; r < t; r = rdtsc())
		pause();
}

int
isdmaok(void *a, usize len, int range)
{
	uintmem pa;

	if(!iskaddr(a) || (char*)a < etext)
		return 0;
	pa = mmuphysaddr(PTR2UINT(a));
	if(pa == 0 || pa == ~(uintmem)0)
		return 0;
	return range > 32 || pa+len <= 0xFFFFFFFFULL;
}
