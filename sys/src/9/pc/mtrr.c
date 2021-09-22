/*
 * memory-type region registers.
 * keep the mtrr sets identical across all cpus.
 * at least 8 variable mtrrs are guaranteed.
 *
 * due to the possibility of extended addresses (for PAE)
 * as large as 36 bits coming from the e820 memory map and the like,
 * we'll use vlongs to hold addresses and lengths, even though we don't
 * implement PAE in Plan 9.
 *
 * MTRR Physical base/mask in MSRs are indexed by
 *	MTRRPhys{Base|Mask}N = MTRRPhys{Base|Mask}0 + 2*N
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

enum {
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
	Uncacheable	= 0,
	Writecomb	= 1,
	Unknown1	= 2,
	Unknown2	= 3,
	Writethru	= 4,
	Writeprot	= 5,
	Writeback	= 6,
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
static Mtrrop *postedop;	/* op in progress across cpus */
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
	memset(regs, 0, sizeof regs);
	cpuid(Exthighfunc, regs);
	if(regs[0] >= Extaddrsz) {			/* ax */
		memset(regs, 0, sizeof regs);
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

/* true if mtrr is valid */
static int
mtrrdec(Mtrreg *mtrr, uvlong *ptr, uvlong *size, int *type)
{
	sanity(mtrr);
	*ptr = PPN(mtrr->base);
	*type = mtrr->base & 0xff;
	*size = (physmask() ^ PPN(mtrr->mask)) + 1;
	return mtrr->mask & Mtrrvalid;
}

static void
mtrrenc(Mtrreg *mtrr, uvlong ptr, uvlong size, int type, int ok)
{
	mtrr->base = ptr | (type & 0xff);
	mtrr->mask = (physmask() & ~(size - 1)) | (ok? Mtrrvalid: 0);
	sanity(mtrr);
}

/*
 * i is the index of the MTRR, and is multiplied by 2 because
 * mask and base offsets are interleaved.
 */
static void
mtrrget(Mtrreg *mtrr, uint i)
{
	if (i >= (m->mtrrcap & Capvcnt))
		error("mtrr index out of range");
	rdmsr(MTRRPhysBase0 + 2*i, &mtrr->base);
	rdmsr(MTRRPhysMask0 + 2*i, &mtrr->mask);
	sanity(mtrr);
}

static void
mtrrput(Mtrreg *mtrr, uint i)
{
	if (i >= (m->mtrrcap & Capvcnt))
		error("mtrr index out of range");
	sanity(mtrr);
	wrmsr(MTRRPhysBase0 + 2*i, mtrr->base);
	wrmsr(MTRRPhysMask0 + 2*i, mtrr->mask);
}

static void
flushtlbs(void)
{
	ulong cr4;

	cr4 = getcr4();
	if (cr4 & CR4pge)
		putcr4(cr4 & ~CR4pge);
	else
		putcr3(getcr3());
}

/*
 * perform an operation (postedop) on an mtrr on this cpu.
 * waits for all the other cpus before starting.
 * nils postedop when done on all cpus.
 * this follows the general outline in intel's x86 vol. 3a, §11.11.8.
 */
static void
mtrrop(void)
{
	int s;
	ulong cr0, cr4;
	static long bar1, bar2;

	s = splhi();		/* avoid race with mtrrclock on this cpu */
	if (postedop == nil) {
		splx(s);
		return;
	}
	/*
	 * wait for all CPUs to sync here, so that the MTRR setup gets
	 * done at roughly the same time on all processors.
	 */
	ainc(&bar1);
	while(bar1 < conf.nmach)
		microdelay(10);
	/* all cpus are now executing this code ~simultaneously */

	cr0 = getcr0();
	cr4 = getcr4();
	wbinvd();
	/* enter no-fill cache mode; maintains coherence */
	putcr0((cr0 | CD) & ~NW);
	wbinvd();
	flushtlbs();

	rdmsr(MTRRCap, &m->mtrrcap);
	rdmsr(MTRRDefaultType, &m->mtrrdef);
	wrmsr(MTRRDefaultType, m->mtrrdef & ~(vlong)Defena); /* disable mtrrs */

	mtrrput(postedop->reg, postedop->slot);	/* change this cpu's mtrr */

	m->mtrrdef |= Defena;
	wrmsr(MTRRDefaultType, m->mtrrdef);	/* enable mtrrs */

	wbinvd();
	flushtlbs();
	putcr0(cr0);				/* restore caches */
	putcr4(cr4);				/* restore paging config */

	/*
	 * wait for all CPUs to sync up again, so that we don't continue
	 * executing while the MTRRs are still being set up.
	 */
	ainc(&bar2);
	while(bar2 < conf.nmach)
		microdelay(10);
	/* all cpus are now executing this code ~simultaneously */

	if (m->machno == 0)
		postedop = nil;
	adec(&bar1);
	while(bar1 > 0)
		microdelay(10);
	/* all cpus are now executing this code ~simultaneously */

	adec(&bar2);
	if (m->machno == 0)
		wakeup(&oprend);
	splx(s);
}

void
mtrrclock(void)				/* called from clock interrupt */
{
	if(postedop != nil)	/* op waiting for this cpu's attention? */
		mtrrop();
}

/* if there's an operation still pending, keep sleeping */
static int
opavail(void *)
{
	return postedop == nil;
}

static int
findmtrr(uvlong base, uvlong size)
{
	int i, vcnt, mtype, mok;
	uvlong mp, msize;
	Mtrreg mtrr;

	vcnt = m->mtrrcap & Capvcnt;
	for(i = 0; i < vcnt; i++){
		mtrrget(&mtrr, i);
		mok = mtrrdec(&mtrr, &mp, &msize, &mtype);
		if(mok && mp == base && msize == size)
			return i;		/* overwrite matching mtrr */
	}
	for(i = 0; i < vcnt; i++){
		mtrrget(&mtrr, i);
		if(!mtrrdec(&mtrr, &mp, &msize, &mtype))
			return i;		/* got a free mtrr */
	}
	for(i = 0; i < vcnt; i++){
		mtrrget(&mtrr, i);
		mok = mtrrdec(&mtrr, &mp, &msize, &mtype);
		/* reuse any entry for addresses above 4GB */
		if(mok && mp >= (1LL<<32))
			return i;		/* got a too-big mtrr */
	}
	return -1;
}

static void
chktype(int type)
{
	switch(type){
	default:
		error("mtrr unknown type");
	case Writecomb:
		if(!(m->mtrrcap & Capwc))
			error("mtrr type wc (write combining) unsupported");
		/* fallthrough */
	case Uncacheable:
	case Writethru:
	case Writeprot:
	case Writeback:
		break;
	}
}

int
mtrr(uvlong base, uvlong size, char *tstr)
{
	int s, type;
	Mtrreg entry;
	Mtrrop op;
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
	chktype(type);

	rdmsr(MTRRCap, &m->mtrrcap);
	rdmsr(MTRRDefaultType, &m->mtrrdef);

	qlock(&mtrrlk);
	if(waserror()){
		qunlock(&mtrrlk);
		nexterror();
	}
	s = splhi();		/* avoid race with mtrrclock on this cpu */
	/* wait for any mtrr op in progress to finish (e.g. from mtrrclock) */
	while(postedop != nil)
		sleep(&oprend, opavail, 0);

	/* find a matching or free mtrr and overwrite it */
	op.slot = findmtrr(base, size);
	if(op.slot < 0)
		error("no free mtrr slots");
	mtrrenc(&entry, base, size, type, 1);
	op.reg = &entry;
	postedop = &op;		/* make visible to other cpus */
	mtrrop();		/* perform synchronised update on all cpus */
	splx(s);

	poperror();
	qunlock(&mtrrlk);
	return 0;
}

int
mtrrprint(char *buf, long bufsize)
{
	int i, vcnt, type;
	long n;
	uvlong base, size;
	Mtrreg mtrr;

	n = 0;
	if(!(m->cpuiddx & Mtrr))
		return 0;
	rdmsr(MTRRCap, &m->mtrrcap);
	rdmsr(MTRRDefaultType, &m->mtrrdef);
	n += snprint(buf+n, bufsize-n, "cache default %s\n",
		type2str(m->mtrrdef & Deftype));
	vcnt = m->mtrrcap & Capvcnt;
	for(i = 0; i < vcnt; i++){
		mtrrget(&mtrr, i);
		if (mtrrdec(&mtrr, &base, &size, &type))
			n += snprint(buf+n, bufsize-n,
				"cache %#llux %llud %s\n",
				base, size, type2str(type));
	}
	return n;
}
