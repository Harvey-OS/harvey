#include "all.h"
#include "mem.h"
#include "io.h"
#include "ureg.h"

#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

/*
 * Invalidate cache lines for the given area.
 */
void
invalcache(ulong v, ulong n)
{
	int s;

	s = splhi();
	v = CACHETAGS+((v % mconf.vacsize) & ~(mconf.vaclinesize-1));	/* BUG */
	n = HOWMANY(n, mconf.vaclinesize);
	while(n){
		putsysspace(v, 0);
		v += mconf.vaclinesize;
		n--;
	}
	splx(s);
}

/*
 * Writeback cache entries to memory.
 * Just do a whole page around the address
 * for now. Only scsi setup at boot does anything
 * other than a whole page.
 */
void
wbackcache(ulong v, ulong n)
{
	ulong s, e, x;

	USED(n);
	x = splhi();
	v = (v & VAMASK) & ~PGOFFSET;
	s = v;
	e = v+PGSIZE;
	do
		s = flushpg(s);
	while(s < e);
	splx(x);
}

void
putpte(ulong virt, ulong phys, int flush)
{
	ulong a, evirt;

	virt = (virt & VAMASK) & ~PGOFFSET;
	if(flush){
		/*
		 * Flush old entries from cache
		 */
		a = virt;
		evirt = virt+PGSIZE;
		do
			a = flushpg(a);
		while(a < evirt);
	}
	putpmegspace(virt, phys);
}

ulong
getpte(ulong virt)
{
	return getpmegspace((virt & VAMASK) & ~PGOFFSET);
}

static void
mmuinit(void)
{
	int i, j;
	ulong pte, ksize;

	ksize = PGROUND((ulong)end);
	ksize = PADDR(ksize);

	/*
	 * Initialise the segments containing the kernel.
	 */
	j = HOWMANY(ksize, SEGSIZE);
	for(i = 0; i < j; i++)
		putcxsegm(0, VADDR(i*SEGSIZE), i);

	/*
	 * Make sure cache is turned on for kernel,
	 * and invalidate anything remaining up to the
	 * next segment boundary.
	 */
	j = HOWMANY(ksize, PGSIZE);
	pte = PTEVALID|PTEWRITE|PTEKERNEL|PTEMAINMEM;
	for(i = 0; i < j; i++)
		putpte(VADDR(i*PGSIZE), pte+PPN(mconf.base0)+i, 1);

	for(i = HOWMANY(ROUNDUP(ksize, SEGSIZE), PGSIZE); j < i; j++)
		putpte(VADDR(j*PGSIZE), INVALIDPTE, 1);

	/*
	 * Initialise an invalid segment.
	 * Use the highest segment of VA.
	 * (Don't really need this).
	 */
	putsegspace(INVALIDSEG, --mconf.npmeg);
	for(i = 0; i < SEGSIZE; i += PGSIZE)
		putpte(INVALIDSEG+i, INVALIDPTE, 1);

	/*
	 * Initialise I/O and kmap region.
	 * Use the segments below the invalid segment.
	 */
	for(i = IOSEGSIZE-SEGSIZE; i >= 0; i -= SEGSIZE){
		putsegspace(IOSEG+i, --mconf.npmeg);
		for(j = 0; j < SEGSIZE; j += PGSIZE)
			putpte(IOSEG+i+j, INVALIDPTE, 1);
	}

	/*
	 * Map remaining segments to virtual addresses
	 * starting at kernel top.
	 * Invalidate all the pte's.
	 */
	for(i = HOWMANY(ksize, SEGSIZE); i < mconf.npmeg; i++){
		putsegspace(VADDR(i*SEGSIZE), i);
		for(j = 0; j < SEGSIZE; j += PGSIZE)
			putpte(VADDR((i*SEGSIZE)+j), INVALIDPTE, 1);
	}

	/*
	 * Mconf.base0 is physical address of 1st page above end;
	 * mconf.npage0 is number of physical pages above
	 * end - see mconfinit below.
	 */

	mconf.npage0 -= ksize/PGSIZE;
	mconf.base0 += ksize;
	mconf.npage = mconf.npage0 + mconf.npage1;

	/*
	 * Initialise the ialloc arena.
	 */
	mconf.vbase = VADDR(ksize);
	mconf.vlimit = mconf.vbase + mconf.npmeg*SEGSIZE;
	if(mconf.vlimit > VADDR(ksize+mconf.npage*PGSIZE))
		mconf.vlimit = VADDR(ksize+mconf.npage*PGSIZE);
}

static struct {
	Lock;
	KMap	*free;
	KMap	arena[IOSEGSIZE/PGSIZE];
} kmapfreelist;

KMap*
kmappa(ulong pa, ulong flag)
{
	KMap *k;
	ulong s;

	lock(&kmapfreelist);
	if((k = kmapfreelist.free) == 0)
		panic("kmappa\n");
	kmapfreelist.free = k->next;
	unlock(&kmapfreelist);
	k->pa = pa;
	s = splhi();
	putpte(k->va, PPN(pa)|PTEVALID|PTEKERNEL|PTEWRITE|flag, 1);
	splx(s);
	return k;
}

void
kunmap(KMap *k)
{
	k->pa = 0;
	lock(&kmapfreelist);
	k->next = kmapfreelist.free;
	kmapfreelist.free = k;
	putpte(k->va, INVALIDPTE, 1);
	unlock(&kmapfreelist);
}

ulong
kmapregion(ulong pa, ulong n, ulong flag)
{
	KMap *k;
	ulong va;
	int i, j;

	/*
	 * kmap's are initially in reverse order so rearrange them.
	 */
	i = HOWMANY(n, PGSIZE);
	va = 0;
	for(j = i-1; j >= 0; j--){
		k = kmappa(pa+j*PGSIZE, flag);
		if(va && va != k->va+PGSIZE)
			panic("kmapregion unordered\n");
		va = k->va;
	}
	return va;
}

void
kmapinit(void)
{
	int howmany, i;
	KMap *k;

	howmany = HOWMANY(IOSEGSIZE, PGSIZE);
	k = kmapfreelist.arena;
	for(i = 0; howmany; howmany--, i += PGSIZE, k++){
		k->va = IOSEG+i;
		kunmap(k);
	}
}

static void
cacheinit(void)
{
	int i;

	/*
	 * Initialize cache by clearing the valid bit
	 * (along with the others) in all cache entries
	 */
	for(i = 0; i < mconf.vacsize; i += mconf.vaclinesize)
		putsysspace(CACHETAGS+i, 0);

	/*
	 * Turn cache on
	 */
	putenab(getenab()|ENABCACHE); /**/
}

void
intrinit(void)
{
#ifdef notdef
	KMap *k;

	k = kmappa(INTRREG, PTEIO|PTENOCACHE);
	intrreg = (uchar*)k->va;
#endif
}

#define MB	(1024*1024)

uchar	idprom[32];
Mconf	mconf;
char	mempres[256];
ulong	rom;			/* open boot rom vector */
ulong	romputcxsegm;
ulong	bank[2];
Label	catch;

typedef struct Sysparam Sysparam;
struct Sysparam
{
	int	id;		/* Model type from id prom */
	char	*name;		/* System name */
	int	usec_delay;	/* clock rate / 3000 */
	char	ss2;		/* Is Sparcstation 2? */
	int	vacsize;	/* Cache size */
	int	vacline;	/* Cache line size */
	int	ncontext;	/* Number of MMU contexts */
	int	npmeg;		/* Number of page map entry groups */
	char	cachebug;	/* Machine needs cache bug work around */
	int	memscan;	/* Number of Meg to scan looking for memory */
}
sysparam[] =
{
	{ 0x51, "1 4/60",    6667, 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x52, "IPC 4/40",  8334, 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x53, "1+ 4/65",   8334, 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x54, "SLC 4/20",  6667, 0, 65536, 16,  8, 128, 0, 64 },
	{ 0x55, "2 4/75",   13334, 1, 65536, 32, 16, 256, 1, 64 },
	{ 0x56, "ELC 4/25", 13334, 1, 65536, 32,  8, 128, 1, 64 },
	{ 0x57, "IPX 4/50", 13334, 1, 65536, 32,  8, 256, 1, 64 },
	{ 0 }
};
Sysparam *sparam;

void
scanmem(char *mempres, int n)
{
	int i;
	ulong va, addr;

	va = 1*MB-2*PGSIZE;
	for(i=0; i<n; i++){
		mempres[i] = 0;
		addr = i*MB;
		putw4(va, PPN(addr)|PTEPROBEMEM);
		*(ulong*)va = addr;
		if(*(ulong*)va == addr){
			addr = ~addr;
			*(ulong*)va = addr;
			if(*(ulong*)va == addr){
				mempres[i] = 1;
				*(ulong*)va = i + '0';
			}
		}
	}
	for(i=0; i<n; i++)
		if(mempres[i]){
			addr = i*MB;
			putw4(va, PPN(addr)|PTEPROBEMEM);
			mempres[i] = *(ulong*)va;
		}else
			mempres[i] = 0;
}

void
mconfinit(void)
{
	ulong i, j;
	ulong va, npg, v;

	/* map id prom */
	va = 1*MB-PGSIZE;
	putw4(va, PPN(EEPROM)|PTEPROBEIO);
	memmove(idprom, (char*)(va+0x7d8), 32);
	if(idprom[0]!=1 || (idprom[1]&0xF0)!=0x50)
		*(ulong*)va = 0;	/* not a sparcstation; die! */
	putw4(va, INVALIDPTE);

	for(sparam = sysparam; sparam->id; sparam++)
		if(sparam->id == idprom[1])
			break;

	/* First entry in the table is the default */
	if(sparam->id == 0)
		sparam = sysparam;

	mconf.usec_delay = sparam->usec_delay;
	mconf.ss2 = sparam->ss2;
	mconf.vacsize = sparam->vacsize;
	mconf.vaclinesize = sparam->vacline;
	mconf.ncontext = sparam->ncontext;
	if(mconf.ncontext > 8)
		mconf.ncontext = 8;	/* BUG to enlarge NKLUDGE */
	mconf.npmeg = sparam->npmeg;
	mconf.ss2cachebug = sparam->cachebug;

	/* Chart memory */
	scanmem(mempres, sparam->memscan);

	/* Find mirrors and allocate banks */
	for(i=0; i<sparam->memscan; i++)
		if(mempres[i]){
			v = mempres[i];
			i = v-'0';
			for(j=i+1; j<sparam->memscan && mempres[j]>v; j++)
				v = mempres[j];
			npg = (j-i)*MB/PGSIZE;
			if(mconf.npage0 == 0){
				mconf.base0 = i*MB;
				mconf.npage0 = npg;
			}else if(mconf.npage1 < npg){
				mconf.base1 = i*MB;
				mconf.npage1 = npg;
			}
			i = v-'0';
		}

	bank[0] = mconf.npage0*PGSIZE/MB;
	bank[1] = mconf.npage1*PGSIZE/MB;

	if(bank[1] == 0){
		/*
		 * This split of memory into 2 banks fools the allocator into
		 * allocating low memory pages from bank 0 for the ethernet since
		 * it has only a 24bit address counter.
		 * NB. Suns must have at LEAST 8Mbytes.
		 */
		mconf.npage1 = mconf.npage0 - (4*MB)/PGSIZE;
		mconf.npage0 = (4*MB)/PGSIZE;
		mconf.base1 = mconf.base0 + 4*MB;
		bank[1] = bank[0]-4;
		bank[0] = 4;
	}

	mconf.npage = mconf.npage0+mconf.npage1;

	romputcxsegm = *(ulong*)(rom+260);
}

ulong
meminit(void)
{
	conf.nmach = 1;
	mconfinit();
	compile();
	mmuinit();
	kmapinit();
	cacheinit();
	intrinit();
	return mconf.npage*PGSIZE;
}

