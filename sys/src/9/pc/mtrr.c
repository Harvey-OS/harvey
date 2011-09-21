/*
 * memory-type region registers.
 *
 * due to the possibility of extended addresses (for PAE)
 * as large as 36 bits coming from the e820 memory map and the like,
 * we'll use vlongs to hold addresses and lengths, even though we don't
 * implement PAE in Plan 9.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

enum {
	/*
	 * MTRR Physical base/mask are indexed by
	 *	MTRRPhys{Base|Mask}N = MTRRPhys{Base|Mask}0 + 2*N
	 */
	MTRRPhysBase0 = 0x200,
	MTRRPhysMask0 = 0x201,
	MTRRDefaultType = 0x2FF,
	MTRRCap = 0xFE,
	Nmtrr = 8,

	/* cpuid extended function codes */
	Exthighfunc = 1ul << 31,
	Extprocsigamd,
	Extprocname0,
	Extprocname1,
	Extprocname2,
	Exttlbl1,
	Extl2,
	Extapm,
	Extaddrsz,

	Paerange = 1LL << 36,
};

enum {
	CR4PageGlobalEnable	= 1 << 7,
	CR0CacheDisable		= 1 << 30,
};

enum {
	Uncacheable	= 0,
	Writecomb	= 1,
	Unknown1	= 2,
	Unknown2	= 3,
	Writethru	= 4,
	Writeprot	= 5,
	Writeback	= 6,
};

enum {
	Capvcnt = 0xff,		/* mask: # of variable-range MTRRs we have */
	Capwc	= 1<<8,		/* flag: have write combining? */
	Capfix	= 1<<10,	/* flag: have fixed MTRRs? */
	Deftype = 0xff,		/* default MTRR type */
	Deffixena = 1<<10,	/* fixed-range MTRR enable */
	Defena	= 1<<11,	/* MTRR enable */
};

typedef struct Mtrreg Mtrreg;
typedef struct Mtrrop Mtrrop;

struct Mtrreg {
	vlong	base;
	vlong	mask;
};
struct Mtrrop {
	Mtrreg	*reg;
	int	slot;
};

static char *types[] = {
[Uncacheable]	"uc",
[Writecomb]	"wc",
[Unknown1]	"uk1",
[Unknown2]	"uk2",
[Writethru]	"wt",
[Writeprot]	"wp",
[Writeback]	"wb",
		nil
};
static Mtrrop *postedop;
static Rendez oprend;

static char *
type2str(int type)
{
	if(type < 0 || type >= nelem(types))
		return nil;
	return types[type];
}

static int
str2type(char *str)
{
	char **p;

	for(p = types; *p != nil; p++)
		if (strcmp(str, *p) == 0)
			return p - types;
	return -1;
}

static uvlong
physmask(void)
{
	ulong regs[4];
	static vlong mask = -1;

	if (mask != -1)
		return mask;
	cpuid(Exthighfunc, regs);
	if(regs[0] >= Extaddrsz) {			/* ax */
		cpuid(Extaddrsz, regs);
		mask = (1LL << (regs[0] & 0xFF)) - 1;	/* ax */
	}
	mask &= Paerange - 1;				/* x86 sanity */
	return mask;
}

/* limit physical addresses to 36 bits on the x86 */
static void
sanity(Mtrreg *mtrr)
{
	mtrr->base &= Paerange - 1;
	mtrr->mask &= Paerange - 1;
}

static int
ispow2(uvlong ul)
{
	return (ul & (ul - 1)) == 0;
}

/* true if mtrr is valid */
static int
mtrrdec(Mtrreg *mtrr, uvlong *ptr, uvlong *size, int *type)
{
	sanity(mtrr);
	*ptr =  mtrr->base & ~(BY2PG-1);
	*type = mtrr->base & 0xff;
	*size = (physmask() ^ (mtrr->mask & ~(BY2PG-1))) + 1;
	return (mtrr->mask >> 11) & 1;
}

static void
mtrrenc(Mtrreg *mtrr, uvlong ptr, uvlong size, int type, int ok)
{
	mtrr->base = ptr | (type & 0xff);
	mtrr->mask = (physmask() & ~(size - 1)) | (ok? 1<<11: 0);
	sanity(mtrr);
}

/*
 * i is the index of the MTRR, and is multiplied by 2 because
 * mask and base offsets are interleaved.
 */
static void
mtrrget(Mtrreg *mtrr, uint i)
{
	if (i >= Nmtrr)
		error("mtrr index out of range");
	rdmsr(MTRRPhysBase0 + 2*i, &mtrr->base);
	rdmsr(MTRRPhysMask0 + 2*i, &mtrr->mask);
	sanity(mtrr);
}

static void
mtrrput(Mtrreg *mtrr, uint i)
{
	if (i >= Nmtrr)
		error("mtrr index out of range");
	sanity(mtrr);
	wrmsr(MTRRPhysBase0 + 2*i, mtrr->base);
	wrmsr(MTRRPhysMask0 + 2*i, mtrr->mask);
}

static void
mtrrop(Mtrrop **op)
{
	int s;
	ulong cr0, cr4;
	vlong def;
	static long bar1, bar2;

	s = splhi();		/* avoid race with mtrrclock */

	/*
	 * wait for all CPUs to sync here, so that the MTRR setup gets
	 * done at roughly the same time on all processors.
	 */
	_xinc(&bar1);
	while(bar1 < conf.nmach)
		microdelay(10);

	cr4 = getcr4();
	putcr4(cr4 & ~CR4PageGlobalEnable);
	cr0 = getcr0();
	wbinvd();
	putcr0(cr0 | CR0CacheDisable);
	wbinvd();
	rdmsr(MTRRDefaultType, &def);
	wrmsr(MTRRDefaultType, def & ~(vlong)Defena);

	mtrrput((*op)->reg, (*op)->slot);

	wbinvd();
	wrmsr(MTRRDefaultType, def);
	putcr0(cr0);
	putcr4(cr4);

	/*
	 * wait for all CPUs to sync up again, so that we don't continue
	 * executing while the MTRRs are still being set up.
	 */
	_xinc(&bar2);
	while(bar2 < conf.nmach)
		microdelay(10);
	*op = nil;
	_xdec(&bar1);
	while(bar1 > 0)
		microdelay(10);
	_xdec(&bar2);
	wakeup(&oprend);
	splx(s);
}

void
mtrrclock(void)				/* called from clock interrupt */
{
	if(postedop != nil)
		mtrrop(&postedop);
}

/* if there's an operation still pending, keep sleeping */
static int
opavail(void *)
{
	return postedop == nil;
}

int
mtrr(uvlong base, uvlong size, char *tstr)
{
	int i, vcnt, slot, type, mtype, mok;
	vlong def, cap;
	uvlong mp, msize;
	Mtrreg entry, mtrr;
	Mtrrop op;
	static int tickreg;
	static QLock mtrrlk;

	if(!(m->cpuiddx & Mtrr))
		error("mtrrs not supported");
	if(base & (BY2PG-1) || size & (BY2PG-1) || size == 0)
		error("mtrr base or size not 4k aligned or zero size");
	if(base + size >= Paerange)
		error("mtrr range exceeds 36 bits");
	if(!ispow2(size))
		error("mtrr size not power of 2");
	if(base & (size - 1))
		error("mtrr base not naturally aligned");

	if((type = str2type(tstr)) == -1)
		error("mtrr bad type");

	rdmsr(MTRRCap, &cap);
	rdmsr(MTRRDefaultType, &def);

	switch(type){
	default:
		error("mtrr unknown type");
		break;
	case Writecomb:
		if(!(cap & Capwc))
			error("mtrr type wc (write combining) unsupported");
		/* fallthrough */
	case Uncacheable:
	case Writethru:
	case Writeprot:
	case Writeback:
		break;
	}

	qlock(&mtrrlk);
	slot = -1;
	vcnt = cap & Capvcnt;
	for(i = 0; i < vcnt && i < Nmtrr; i++){
		mtrrget(&mtrr, i);
		mok = mtrrdec(&mtrr, &mp, &msize, &mtype);
		/* reuse any entry for addresses above 4GB */
		if(!mok || mp == base && msize == size || mp >= (1LL<<32)){
			slot = i;
			break;
		}
	}
	if(slot == -1)
		error("no free mtrr slots");

	while(postedop != nil)
		sleep(&oprend, opavail, 0);
	mtrrenc(&entry, base, size, type, 1);
	op.reg = &entry;
	op.slot = slot;
	postedop = &op;
	mtrrop(&postedop);
	qunlock(&mtrrlk);
	return 0;
}

int
mtrrprint(char *buf, long bufsize)
{
	int i, vcnt, type;
	long n;
	uvlong base, size;
	vlong cap, def;
	Mtrreg mtrr;

	n = 0;
	if(!(m->cpuiddx & Mtrr))
		return 0;
	rdmsr(MTRRCap, &cap);
	rdmsr(MTRRDefaultType, &def);
	n += snprint(buf+n, bufsize-n, "cache default %s\n",
		type2str(def & Deftype));
	vcnt = cap & Capvcnt;
	for(i = 0; i < vcnt && i < Nmtrr; i++){
		mtrrget(&mtrr, i);
		if (mtrrdec(&mtrr, &base, &size, &type))
			n += snprint(buf+n, bufsize-n,
				"cache 0x%llux %llud %s\n",
				base, size, type2str(type));
	}
	return n;
}
