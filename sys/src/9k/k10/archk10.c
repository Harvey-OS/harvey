#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

typedef struct Inmodels Inmodels;
struct Inmodels {
	ushort	num;
	char	*name;
};

/*
 * wait for *vp to change from val, or for an interrupt, on machines
 * that lack monitor & mwait instructions.
 * shouldn't be needed on hardware; VMs might need it.
 */
static int
portwaitfor(int *vp, int val)
{
	int i;
	Proc *restpl;
	Mpl s;

	if (sys->nonline <= 1)
		halt();
	else {
		s = 0;
		restpl = up;
		if (up)	/* this cpu is scheduling, thus done initialising? */
			s = spllo();
		/*
		 * How many times round this loop?
		 */
		for(i = 10000; *vp == val && i > 0; i--)
			pause();
		if (restpl)
			splx(s);
	}
	return *vp;
}

/* interface to the port code */
void
portmonitor(void *a)
{
	if (waitfor == k10waitfor)
		monitor(a, 0, 0);
}

/*
 * only if mwait can be interrupted will we be able to
 * detect lock loops at high PL.
 *
 * this may seem to be a form of optimizing the idle loop, but mwait
 * should consume less power and fewer instructions on many-core systems
 * with more lock contention (e.g., NPROC=24 mk).
 */
void
portmwait(void)
{
	if(sys->haveintrbreaks)
		mwait(Intrbreaks, 0);
	else if (islo())
		mwait(0, 0);
}

int (*waitfor)(int*, int) = portwaitfor;
void (*mwaitforlock)(Lock *, ulong) = portmwaitforlock;

static int
cpuidinit(void)
{
	int vabits;
	u32int eax, info[4];

	/*
	 * Standard CPUID functions.
	 * Functions 0 and 1 will be needed multiple times
	 * so cache the info now.
	 */
	if((m->ncpuinfos = cpuid(Highstdfunc, 0, m->cpuinfo[Highstdfunc])) == 0)
		return 0;
	m->ncpuinfos++;

	if(memcmp(&m->cpuinfo[Highstdfunc][Bx], "GenuntelineI", 12) == 0)
		m->isintelcpu = 1;
	cpuid(Procsig, 0, m->cpuinfo[Procsig]);

	/*
	 * Extended CPUID functions.
	 */
	if((eax = cpuid(Highextfunc, 0, info)) >= Highextfunc)
		m->ncpuinfoe = (eax & ~Highextfunc) + 1;

	if (m->machno != 0)
		return 1;

	/*
	 * Is MONITOR/MWAIT supported?
	 * All actual k10 and later hardware has them.
	 */
	if(m->cpuinfo[Procsig][Cx] & Monitor) {
		waitfor = k10waitfor;
		mwaitforlock = realmwaitforlock;
		if(cpuidinfo(Procmon, 0, info) && info[Cx] & Alwaysbreak)
			sys->haveintrbreaks = 1;
	} else {				/* pre-2014 pc or emulation */
		vmbotch(0, "monitor & mwait instructions disabled; "
			"adjust your bios settings");
		delay(500);
	}

	if(cpuidinfo(Extfeature, 0, info) && (info[Dx] & Nx) == 0)
		print("no no-execute bit support!\n");

	if(cpuidinfo(Extcap, 0, info)) {
		vabits = (info[Ax]>>8) & MASK(8);
		if (vabits != 48)		/* 48 is standard */
			print("%d-bit virtual addresses supported\n", vabits);
	}
	if(cpuidinfo(Procid7, 0, info) && info[Cx] & L5pt)
		print("cpu supports 5-level page tables (but unused)\n");
	delay(200);			/* allow reading before scrolling */
	return 1;
}

int
cpuidinfo(u32int eax, u32int ecx, u32int info[4])
{
	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return 0;

	if(!(eax & Highextfunc)){
		if(eax >= m->ncpuinfos)
			return 0;
	} else if(eax >= (Highextfunc|m->ncpuinfoe))
		return 0;

	cpuid(eax, ecx, info);
	return 1;
}

enum {
	Monitorsz = 64,			/* typical, but much bigger in VMs */
};

static int mwords[(Monitorsz/BY2WD + 1)*2];	/* isolated cache line */
static int *mwaitwd = mwords;			/* make safe from the start */
static int mwdaligned;

void
mwdalign(void)
{
	uint min, max;
	uint info[4];

	if (mwdaligned || m == nil || m->machno != 0)
		return;

	/* not yet aligned, and on cpu0 */
	if (!mallocok || m->ncpuinfos == 0 || Procmon >= m->ncpuinfos) {
		mwaitwd = (int *)ROUNDUP((uintptr)mwords, Monitorsz);
		if (m->ncpuinfos > 0 && Procmon >= m->ncpuinfos)
			mwdaligned = 1;	/* Procmon too big; will never work */
		return;
	}

	/* ok to malloc, and can get monitor min & max sizes */
	memset(info, 0, sizeof info);
	max = min = Monitorsz;			/* defaults */
	if(cpuidinfo(Procmon, 0, info)) {
		min = info[Ax] & MASK(16);	/* wakeup-write region size */
		max = info[Bx] & MASK(16);	/* avoid-false-wakeup size */
		if (min == 0 || max == 0 || max < min)
			max = min = Monitorsz;	/* garbled results, default */
	}
	mwdaligned = 1;
	if (max > Monitorsz && up && islo())
		mwaitwd = mallocalign(max, min, 0, 0);
}

/*
 *  put the processor in the halt state if we've no processes to run.
 *  an interrupt or change to *mwaitwd will get us going again.
 *  called at spllo from proc.c, but also in infinite loops elsewhere.
 */
void
idlehands(void)
{
	if (!mwdaligned)
		mwdalign();
	waitfor(mwaitwd, *mwaitwd);
}

/*
 * wake any cpus in mwait state.  an interrupt will make one cpu break
 * out of mwait, but a qunlock probably should wake any idlers too.
 */
void
idlewake(void)
{
	if (!mwdaligned)
		mwdalign();
	aadd(mwaitwd, 1);		/* not ainc, since this could wrap */
	coherence();
}

static vlong intelhz[] = {
	266666666666ll, 133333333333ll, 200000000000ll,
	166666666666ll, 333333333333ll, 100000000000ll,
	400000000000ll,
};

/* Xeons implement ECC memory; no other Intel models do. */
static Inmodels in6models[] = {
	0x1e, "xeon/core i5/i7",	/* 2009 */
	0x1f, "xeon/core i7",
	0x22, "core i7",
	0x25, "xeon 3000/core i3/i5/i7",
	0x26, "atom e6xx",
	0x2a, "xeon e3/core i3/i5/i7",
	0x2c, "xeon 3000/core i7",
	0x2d, "xeon e5/core i7",
	0x2e, "xeon 6000 mp",
	0x2f, "xeon e7 mp",
	0x3a, "xeon e3/core i3/i5/i7",	/* ~2012 */
	0x3c, "xeon/core i7",
	0x3d, "core (i7|m)-5557u nuc",
	0x3e, "core i7",
	0x3f, "core i7",
	0x9e, "xeon e3-1275 v6",	/* ~2017 */
};

static vlong
intelfamfhz(int fammod, int mod)
{
	int r;
	vlong hz;
	u64int msr;

	hz = 0;
	switch(mod){
	default:
		print("unknown intel family F cpu model; id %#ux\n", fammod);
		break;
	case 3:		/* Xeon (MP), Pentium [4D] */
	case 4:		/* Xeon (MP), Pentium [4D] */
	case 6:		/* Xeon 7100, 5000 or above */
		print("intel family F: xeon or pentium [4D]");
		msr = rdmsr(Fsbfreqr);
		r = (msr>>16) & MASK(3);
		if ((uint)r >= nelem(intelhz)) {
			print("unknown Intel cpu speed, msr %#x = %#llux\n",
				Fsbfreqr, msr);
			break;
		}

		/*
		 * intelhz[r] is *1000 at this point.
		 * Do the scaling then round it.
		 * The manual is conflicting about
		 * the size of the msr field.
		 */
		hz = ((intelhz[r]*(msr>>24))/100 + 5)/10;
		/* is hz actual or nominal? */
		DBG("cpuidhz: %#x: %#llux, hz %lld\n", Fsbfreqr, msr, hz);
		break;
	}
	return hz;
}

static vlong
intelfam6hz(int fammod, int mod, u32int info[2][4])
{
	int r, f;
	vlong hz;
	u64int msr;
	Inmodels *im, *imend;

	hz = 0;
	switch(mod){
	case 0x09:		/* Pentium M, Celeron M */
	case 0x0d:		/* Pentium M, Celeron M */
		print("intel pentium M or celeron M");
		hz = ((rdmsr(Pwroncfg)>>22) & MASK(5))*100 * 1000000ll;
		/* is hz actual or nominal? */
		break;
	case 0x0e:		/* Core Duo */
	case 0x0f:		/* Core 2 Duo/Quad/Extreme */
	case 0x17:		/* Core 2 Extreme */
		print("intel core");
		/*
		 * Get the FSB frequemcy.
		 * If processor has Enhanced Intel Speedstep Technology
		 * then non-integer bus frequency ratios are possible.
		 */
		if(info[Procsig][Cx] & Eist){
			msr = rdmsr(0x198);
			r = msr>>40;
		} else {
			msr = 0;
			r = rdmsr(Pwroncfg);
		}
		r &= MASK(5);
		f = rdmsr(0xcd) & MASK(3);
		if ((uint)r >= nelem(intelhz)) {
			print("unknown Intel cpu speed, msr 0xcd %#ux\n", f);
			break;
		}
		hz = intelhz[r];

		/*
		 * Hz is *1000 at this point.
		 * Do the scaling then round it.
		 */
		hz = hz*r + ((msr & (1ull << 46))? hz/2: 0);
		hz = (hz/100 + 5)/10;
		/* is hz actual or nominal? */
		DBG("cpuidhz: %#x: %#llux hz %lld\n",
			Pwroncfg, rdmsr(Pwroncfg), hz);
		break;
	default:
		/* a cpu we don't know msrs for.  do we know its name? */
		imend = in6models + nelem(in6models);
		for (im = in6models; im < imend; im++)
			if (im->num == mod)
				break;
		if (im < imend)
			print("intel %s", im->name);
		else
			print("unknown Intel family 6 cpu model %d; id %#ux\n",
				mod, fammod);
		break;
	}
	return hz;
}

/*
 * return value of 0 means we couldn't figure out speed from msrs,
 * typically because we don't know which msrs are available for a given model.
 */
static vlong
intelcpuhz(u32int info[2][4])
{
	int fammod, fam, mod;
	uvlong hz;

	fammod = info[Procsig][Ax] & 0x0fff3ff0;	/* ignore stepping */
	fam = X86FAMILY(fammod);
	mod = X86MODEL(fammod);
	if (fam != 6 && fam != 0xf) {
		print("unknown Intel cpu family %#ux\n", fam);
		return 0;
	}
	if (fam == 0xf)			/* some MP xeons, misc. Pentia */
		hz = intelfamfhz(fammod, mod);
	else
		hz = intelfam6hz(fammod, mod, info);	/* common case */
	if (hz != 0)
		print(" %,lld MHz nominal;", (hz + 500000) / 1000000);
	return hz;
}

static vlong
amdcpuhz(u32int info[2][4])
{
	int r, fammod, mhz;
	vlong hz;
	u64int msr;
	char *model;

	model = nil;
	fammod = info[Procsig][Ax] & 0x0fff0ff0;
	switch(fammod){
	default:
		print("unknown AMD cpu family & model %#ux\n", fammod);
		return 0;
	case 0x00000f50:		/* K8 */
		if (model == nil)
			model = "k8";
		msr = rdmsr(0xc0010042);
		if(msr == 0) {
			print("AMD K8 zero msr 0xc0010042\n");
			return 0;
		}
		hz = (800 + 200*((msr>>1) & 0x1f)) * 1000000ll;
		break;
	case 0x00100f90:		/* K10 */
		if (model == nil)
			model = "k10";
	case 0x00730f00:		/* Jaguar */
		if (model == nil)
			model = "jaguar";
	case 0x00000620:		/* QEMU64 */
		if (model == nil)
			model = "emulated qemu64";
		msr = rdmsr(0xc0010064);
		r = (msr>>6) & 0x07;
		hz = (((msr & 0x3f)+0x10)*100000000ll)/(1<<r);
		break;
	}
	DBG("cpuidhz: %#llux hz %lld\n", msr, hz);
	mhz = (hz + 500000) / 1000000;
	print("amd %s cpu(s) at %,d MHz nominal;", (model? model: ""), mhz);
	return hz;
}

static vlong
cpuidhz(u32int info[2][4])
{
	if(memcmp(&info[Highstdfunc][Bx], "GenuntelineI", 12) == 0)
		return intelcpuhz(info);
	else if(memcmp(&info[Highstdfunc][Bx], "AuthcAMDenti", 12) == 0)
		return amdcpuhz(info);
	else {
		print("unknown cpu manufacturer\n");
		return 0;
	}
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
		cpuid(Highextfunc|i, 0, info);
		DBG("eax = %#8.8ux: %8.8ux %8.8ux %8.8ux %8.8ux\n",
			Highextfunc|i, info[0], info[1], info[2], info[3]);
	}
}

vlong
archhz(void)
{
	vlong nomhz, acthz;
	u32int info[Procsig+1][Dx+1];

	if(DBGFLG && m->machno == 0)
		cpuiddump();
	if(!cpuidinfo(Highstdfunc, 0, info[Highstdfunc]) ||
	   !cpuidinfo(Procsig, 0, info[Procsig]))
		return 0;

	if(m->machno != 0)
		return sys->machptr[0]->cpuhz;	/* cpus have to be identical */
	nomhz = cpuidhz(info);
	acthz = i8254hz(info);
	return acthz? acthz: nomhz;
}

static void
addpgsz(int lg2)
{
	int npg;

	assert(m->npgsz < NPGSZ);
	npg = m->npgsz++;
	m->pgszlg2[npg] = lg2;
	m->pgszmask[npg] = MASK(lg2);
}

int
archmmu(void)
{
	u32int info[4];

	/*
	 * Should the check for m->machno != 0 be here
	 * or in the caller (mmuinit)?
	 *
	 * Pge and Pae in Cr4 were set in _lme().
	 */

	/*
	 * How many page sizes are there?
	 * Always have 4K, but need to check
	 * configured correctly.
	 */
	CTASSERT(PGSZ == 4*KB, PGSZ_4K);
	addpgsz(12);

	/*
	 * Check the Psep bit in function 1 DX for 2MB support;
	 * if false, only 4K is available.
	 */
	if(m->ncpuinfos == 0 && cpuidinit() == 0)
		return 1;
	if(!(m->cpuinfo[Procsig][Dx] & Psep))
		return 1;
	addpgsz(21);

	if(cpuidinfo(Extfeature, 0, info) && (info[Dx] & Page1gb))
		addpgsz(30);
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
	return fmtprint(f, "%#16.16llux", va_arg(f->args, Mpl));
}

static int
fmtR(Fmt* f)
{
	return fmtprint(f, "%#16.16llux", va_arg(f->args, u64int));
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

/* delay for at least microsecs, avoiding the watchdog if necessary */
void
microdelay(vlong microsecs)
{
	uvlong t, now, nxtdog;

	if (microsecs <= 0)
		return;
	now = rdtsc();
	t = now + (vlong)m->cpumhz*microsecs;
	/* when islo(), clock interrupts will restart the dog */
	if (watchdogon && m->machno == 0 && !islo()) {
		nxtdog = 0;
		while ((now = rdtsc()) < t) {
			if (now > nxtdog) {
				watchdog->restart();
				nxtdog = now + Wdogms*1000ll*m->cpumhz;
			}
			pause();
		}
	} else
		while (rdtsc() < t)
			pause();
}

void
millidelay(int millisecs)
{
	microdelay(1000ll * millisecs);
}

int
clz(Clzuint n)			/* count leading zeroes */
{
	if (n == 0)
		return Clzbits;
	return Clzbits - 1 - bsr(n);
}
