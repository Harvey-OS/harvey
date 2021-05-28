#include "include.h"
#include "fs.h"

enum {
	Dontprint,
	Print,

	Datamagic	= 0xfacebabe,
};

int securemem;

static char *etherparts[] = { "*", 0 };
static char *etherinis[] = {
	"/cfg/pxe/%E",
	0
};

Type types[] = {
	{	Tether,
		Fini|Fbootp,
		etherinit, etherinitdev,
		pxegetfspart, 0, bootpboot,
		etherprintdevs,
		etherparts,
		etherinis,
	},
};

static char *typenm[] = {
	[Tether]	"ether",
};

typedef struct Mode Mode;

enum {
	Dany		= -1,
	Nmedia		= 2,		/* size reduction; was 16 */
};

enum {					/* mode */
	Mauto,
	Mlocal,
	Manual,
	NMode,
};

typedef struct Medium Medium;
struct Medium {
	Type*	type;
	ushort	flag;
	ushort	dev;
	char	name[NAMELEN];

	Fs	*inifs;
	char	*part;
	char	*ini;

	Medium*	next;
};

typedef struct Mode {
	char*	name;
	ushort	mode;
} Mode;

extern char bdata[], edata[], end[], etext[];

static Medium media[Nmedia];
static Medium *curmedium = media;

static Mode modes[NMode+1] = {
	[Mauto]		{ "auto",   Mauto,  },
	[Mlocal]	{ "local",  Mlocal, },
	[Manual]	{ "manual", Manual, },
};

Mach* machptr[MAXMACH];

ulong cpuentry = 0;

char **ini;
char *defaultpartition;
char *persist;

int debugload;
int iniread;
int pxe = 1;
int scsi0port;
int vga;

uintptr memstart;				/* set before main called */
uintptr vectorbase;				/* set before main called */

static uintptr memsz;

static Medium*
parse(char *line, char **file)
{
	char *p;
	Type *tp;
	Medium *mp;

	if(p = strchr(line, '!')) {
		*p++ = 0;
		*file = p;
	} else
		*file = "";

	tp = types;
	for(mp = tp->media; mp; mp = mp->next)
		if(strcmp(mp->name, line) == 0)
			return mp;
	if(p)
		*--p = '!';
	return nil;
}

static int
boot(Medium *mp, char *file)
{
	Type *tp;
	Medium *xmp;
	static int didaddconf;
	Boot b;

	memset(&b, 0, sizeof b);
	b.state = INITKERNEL;

	if(didaddconf == 0) {
		didaddconf = 1;
		tp = types;
		if(tp->addconf)
			for(xmp = tp->media; xmp; xmp = xmp->next)
				(*tp->addconf)(xmp->dev);
	}

	seprint(BOOTLINE, BOOTLINE + BOOTLINELEN, "%s!%s", mp->name, file);
	print("booting %s!%s\n", mp->name, file);
	return (*mp->type->boot)(mp->dev, file, &b);
}

static Medium*
allocm(Type *tp)
{
	Medium **l;

	if(curmedium >= &media[Nmedia])
		return 0;

	for(l = &tp->media; *l; l = &(*l)->next)
		;
	*l = curmedium++;
	return *l;
}

Medium*
probe(int type, int flag, int dev)
{
	Type *tp;
	int i;
	Medium *mp;
	File f;
	Fs *fs;
	char **partp;

	tp = types;
	if(type != Tany && type != tp->type)
		return 0;

	if(flag != Fnone)
		for(mp = tp->media; mp; mp = mp->next)
			if((flag & mp->flag) && (dev == Dany || dev == mp->dev))
				return mp;
	if((tp->flag & Fprobe) == 0){
		tp->flag |= Fprobe;
		tp->mask = (*tp->init)();
	}

	for(i = 0; tp->mask; i++){
		if((tp->mask & (1<<i)) == 0)
			continue;
		tp->mask &= ~(1<<i);

		if((mp = allocm(tp)) == 0)
			continue;

		mp->dev = i;
		mp->flag = tp->flag;
		mp->type = tp;
		(*tp->initdev)(i, mp->name);

		if(mp->flag & Fini){
			mp->flag &= ~Fini;
			for(partp = tp->parts; *partp; partp++){
				if((fs = (*tp->getfspart)(i, *partp, 0)) == nil)
					continue;

				for(ini = tp->inis; *ini; ini++)
					if(fswalk(fs, *ini, &f) > 0){
						mp->inifs = fs;
						mp->part = *partp;
						mp->ini = f.path;
						mp->flag |= Fini;
						goto Break2;
					}
			}
		}
	Break2:
		if((flag & mp->flag) && (dev == Dany || dev == i))
			return mp;
	}
	return 0;
}

enum {
	Kilo = 1024,
};

/* make sure we don't get the write system call from libc */
long
write(int fd, void *buf, long nbytes)
{
	USED(fd);
	vuartputs(buf, nbytes);
	return nbytes;
}

/*
 * write zero words to an entire cache line (the one containing addr).
 * does not flush the data cache.
 */
void
cacheline0(uintptr addr)
{
	ulong *sp, *endmem;

	addr &= ~(DCACHELINESZ - 1);
	endmem = (ulong *)(addr + DCACHELINESZ);
	for (sp = (ulong *)addr; sp < endmem; sp++)
		*sp = 0;
//	coherence();
}

/* force the four qtm write buffers to be retired to dram by filling them. */
void
flushwrbufs(void)
{
	if (!securemem)
		return;
	cacheline0(PHYSDRAM);
	cacheline0(PHYSDRAM + DCACHELINESZ);
	cacheline0(PHYSDRAM + 2*DCACHELINESZ);
	cacheline0(PHYSDRAM + 3*DCACHELINESZ);
	coherence();
}

static void
vfyzeros(ulong *addr, ulong *end)
{
	ulong wd;
	ulong *sp;

	for (sp = addr; sp < end; sp++) {
		wd = *sp;
		if (wd != 0) {
			PROG('?')
			panic("bad dram: %#p read back as %#lux not 0", sp, wd);
		}
	}
}

static int
memreset(void)
{
	int i, cnt;
	uintptr addr, endmem;

	cnt = 0;
	if (securemem)
		cnt = qtmmemreset();
	else
		/* normal dram init. should take 100—250 ms. */
		for (i = 10*1000*1000; i-- > 0; )
			cnt++;
PROG('t')
	/*
	 * dram must be done initialising now,
	 * but qtm needs us to zero it *all* before any other use,
	 * to set the macs.  even for non-qtm dram, it might be wise
	 * to ensure that all the ecc bits are set.
	 */
	memsz = memsize();		/* may carefully probe qtm dram */
	qtmerrtest("sizing memory");

	dcflush(PHYSSRAM, (1ULL << 32) - PHYSSRAM);
	cachesinvalidate();

	endmem = PHYSDRAM + memsz;
	for (addr = PHYSDRAM; addr < endmem; addr += DCACHELINESZ) {
		cacheline0(addr);
		qtmerrtestaddr(addr);
	}
	assert(addr == endmem);
	coherence();
	flushwrbufs();
	qtmerrtest("zeroing dram");

#ifdef PARANOID
	vfyzeros((ulong *)PHYSDRAM, (ulong *)endmem);
#else
	vfyzeros((ulong *)(endmem - MB), (ulong *)endmem);
#endif
	dcflush(PHYSDRAM, memsz);
	dcflush(PHYSSRAM, (1ULL << 32) - PHYSSRAM);
	cachesinvalidate();

	/*
	 * Hallelujah!  We can finally treat qtm dram just like real memory,
	 * except that the last (partial) page is funny and should be avoided
	 * after initialising it.
	 * It happens even with caches off, so it's a bit hard to explain
	 * why it should be funny.
	 */
	memsz &= ~(BY2PG - 1);
	endmem = PHYSDRAM + memsz;

#ifdef PARANOID
	vfyzeros((ulong *)PHYSDRAM, (ulong *)endmem);
#else
	vfyzeros((ulong *)PHYSDRAM, (ulong *)(PHYSDRAM + MB));
	vfyzeros((ulong *)(endmem - MB), (ulong *)endmem);
#endif
	qtmerrtest("reading back dram");
	return cnt;
}

static void
memtest(void)
{
	ulong wd;
	ulong *sp, *endmem;

	/*
	 * verify that (possibly secure) dram is more or less working.
	 * write address of each word into that word.
	 */
#ifdef PARANOID
	endmem = (ulong *)(PHYSDRAM + memsz);
#else
	endmem = (ulong *)(PHYSDRAM + MB);		/* just the first MB */
#endif
	for (sp = (ulong *)PHYSDRAM; sp < endmem; sp++)
		*sp = (ulong)sp;
	coherence();
	/* no need to flush caches, caches are off */

	/*
	 * now verify that each word contains its address.
	 */
	for (sp = (ulong *)PHYSDRAM; sp < endmem; sp++) {
		wd = *sp;
		if (wd != (ulong)sp) {
			PROG('?')
			panic("bad dram: %#p read back as %#lux", sp, wd);
		}
	}
PROG('t')
/*	memset((void *)PHYSDRAM, 0, memsz);	/* good hygiene? */
}

enum {
	Testbase = PHYSDRAM + 0x4000,
};

void
meminit(void)
{
	int i;
	uchar *memc;
	ulong *mem;

//	iprint("sanity: writing...");
	mem  = (ulong *)Testbase;
	memc = (uchar *)Testbase;

	memset(mem, 0252, Kilo);
	coherence();
	dcflush(Testbase, Kilo);

//	iprint("reading...");
	for (i = 0; i < Kilo; i++)
		if (memc[i] != 0252)
			panic("dram not retaining data");

//	iprint("zeroing...");
	memset(mem, '\0', Kilo);
	coherence();
	dcflush(Testbase, Kilo);
	if (*mem)
		panic("zeroed dram not zero");
//	iprint("\n");
}

static int idx;
static char numb[32];

static void
gendigs(ulong n)
{
	int dig;

	do {
		dig = n % 16;
		if (dig > 9)
			numb[idx--] = 'A' + dig - 10;
		else
			numb[idx--] = '0' + dig;
	} while ((n /= 16) > 0);
}

static void
prnum(ulong n)
{
//	PROG(' ')
	idx = nelem(numb) - 1;
	gendigs(n);
	for (; idx < nelem(numb); idx++)
		PROG(numb[idx])
	PROG('\n')
	PROG('\r')
}

extern uintptr vectorbase;

void
main(void)
{
	int flag, i, j, mode, tried;
	char def[2*NAMELEN], line[80], *p, *file;
	Medium *mp;
	Type *tp;
	static int savcnt, caching;
	static ulong vfy = Datamagic;

	/*
	 * all cpus start executing at reset address, thus start of sram.
	 * cpu0 loads the kernel;
	 * all others just jump to the (by now) loaded kernel.
	 */
	/* "\r\nPlan " already printed by l.s */
	if (getpir() != 0) {
		for (j = 0; j < 6; j++)
			for (i = 2*1000*1000*1000; i > 0; i--)
				;

		cachesinvalidate();
		while(cpuentry == 0)
			cachesinvalidate();

		for (j = 0; j < 6; j++)
			for (i = 2*1000*1000*1000; i > 0; i--)
				;
		warp9(PADDR(cpuentry));
	}

	/*
	 * we may have to realign the data segment; apparently ql -R4096
	 * does not pad the text segment.
	 */
	if (vfy != Datamagic)
		memmove(bdata, etext, edata - bdata);
	if (vfy != Datamagic) {
		PROG('?')
		panic("misaligned data segment");
	}
//	memset(edata, 0, end - edata);		/* zero bss */

PROG('9')
	/*
	 * trap vectors are in sram, so we don't have to wait for dram
	 * to become ready to initialise them.
	 */
	trapinit();
PROG(' ')
	securemem = (probeaddr(Qtm) >= 0);
	if (securemem)
		PROG('q')
	else
		PROG('n')
PROG(' ')

	/*
	 * the stack is now at top of sram, and entry to main just pushed
	 * stuff onto it.  the new stack will be at the top of dram,
	 * when dram finishes initialising itself.
	 */
PROG('B')
PROG('o')
PROG('o')
	/* do voodoo to make dram usable; prints "t".  sets memsz. */
	savcnt = memreset();
PROG('s')
	memtest();			/* also prints "t" */
	intrinit();

	caching = cacheson();

	/*
	 * switch to the dram stack just below the end of dram.
	 * have to allow enough room for main's local variables,
	 * to avoid address faults.
	 */

PROG('r')
	setsp(memsz - BY2PG);

PROG('a')
	meminit();
	/* memory is now more or less normal from a software perspective. */

	memset(m, 0, sizeof(Mach));
	m->machno = 0;
	m->up = nil;
	MACHP(0) = m;

	/*
	 * the Mach struct is now initialised, so we can print safely.
	 */

//	print("\nPlan 9 bootstrap");	/* already printed, 1 char at a time */
	print("p");
//	print("\n%d iterations waiting for %s init done\n",
//		savcnt, (securemem? "qtm": "dram"));
	if (securemem)
		print("; secure memory");
	print("; caches %s\n", (caching? "on": "off"));
	print("\n");
	if ((uintptr)end < PHYSSRAM)
		panic("too big; end %#p before sram @ %#ux", end, PHYSSRAM);
	print("memory found: %,lud (%lux)\n", memsz, memsz);

	spllo();
	clockinit();
	prcpuid();
//	etherinit();			/* probe() calls this */
	kbdinit();

	/*
	 * find and read plan9.ini, setting configuration variables.
	 */
	tp = types;
//	debug = debugload = 1;		// DEBUG
	if(pxe && (mp = probe(tp->type, Fini, Dany)) && mp->flag & Fini){
		if (debug)
			print("using %s!%s!%s\n", mp->name, mp->part, mp->ini);
//		iniread = !dotini(mp->inifs);
	}

	/*
	 * we should now have read plan9.ini, if any.
	 */
	persist = getconf("*bootppersist");

	tried = 0;
	mode = Mauto;

	p = getconf("bootfile");
	if(p != 0) {
		mode = Manual;
		for(i = 0; i < NMode; i++){
			if(strcmp(p, modes[i].name) == 0){
				mode = modes[i].mode;
				goto done;
			}
		}
		if((mp = parse(p, &file)) == nil)
			print("Unknown boot device: %s\n", p);
		else
			tried = boot(mp, file);
	}
done:
	if(tried == 0 && mode != Manual){
		flag = Fany;
		if(mode == Mlocal)
			flag &= ~Fbootp;
		if((mp = probe(Tany, flag, Dany)))
			boot(mp, "");
		if (debugload)
			print("end auto probe\n");
	}

	def[0] = 0;
	if(p = getconf("bootdef"))
		strecpy(def, def + sizeof def, p);
	flag = 0;
	tp = types;
	for(mp = tp->media; mp; mp = mp->next){
		if(flag == 0){
			flag = 1;
			print("Boot devices:");
		}
		(*tp->printdevs)(mp->dev);
	}
	if(flag)
		print("\n");

	/*
	 * e.g., *bootppersist=ether0
	 *
	 * previously, we looped in bootpopen if we were pxeload or if
	 * *bootppersist was set.  that doesn't work well for pxeload where
	 * bootp will never succeed on the first interface but only on another
	 * interface.
	 */
//print("boot mode %s\n", modes[mode].name);
	if (mode == Mauto && persist != nil &&
	    (mp = parse(persist, &file)) != nil) {
		boot(mp, file);
		print("pausing before retry...");
		delay(30*1000);
		print("\n");
	}

	for(;;){
		if(getstr("boot from", line, sizeof(line), def,
		    (mode != Manual)*15) >= 0)
			if(mp = parse(line, &file))
				boot(mp, file);
		def[0] = 0;
	}
}

int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i = 0; lp && *lp && i < n; i++){
		while(*lp == sep)
			*lp++ = 0;
		if(*lp == 0)
			break;
		fields[i] = lp;
		while(*lp && *lp != sep){
			if(*lp == '\\' && *(lp+1) == '\n')
				*lp++ = ' ';
			lp++;
		}
	}
	return i;
}

int
cistrcmp(char *a, char *b)
{
	int ac, bc;

	for(;;){
		ac = *a++;
		bc = *b++;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');
		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}
	return 0;
}

int
cistrncmp(char *a, char *b, int n)
{
	unsigned ac, bc;

	while(n > 0){
		ac = *a++;
		bc = *b++;
		n--;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');

		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}

	return 0;
}

#define PSTART		(8*MB)		/* start allocating here */
#define PEND		(100*MB)	/* below stack */

static ulong palloc = PSTART;

void*
ialloc(ulong n, int align)
{
	ulong p;
	int a;

	assert(palloc >= PSTART);
	p = palloc;
	if(align <= 0)
		align = 4;
	if(a = n % align)
		n += align - a;
	if(a = p % align)
		p += align - a;

	palloc = p+n;
	if(palloc > PEND)
		panic("ialloc(%lud, %d) called from %#p",
			n, align, getcallerpc(&n));
	return memset((void*)p, 0, n);
}

void*
xspanalloc(ulong size, int align, ulong span)
{
	ulong a, v;

	if((palloc + (size+align+span)) > PEND)
		panic("xspanalloc(%lud, %d, 0x%lux) called from %#p",
			size, align, span, getcallerpc(&size));

	a = (ulong)ialloc(size+align+span, 0);

	if(span > 2)
		v = (a + span) & ~(span-1);
	else
		v = a;

	if(align > 1)
		v = (v + align) & ~(align-1);

	return (void*)v;
}

static Block *allocbp;

Block*
allocb(int size)
{
	Block *bp, **lbp;
	ulong addr;

	lbp = &allocbp;
	for(bp = *lbp; bp; bp = bp->next){
		if((bp->lim - bp->base) >= size){
			*lbp = bp->next;
			break;
		}
		lbp = &bp->next;
	}
	if(bp == 0){
		if((palloc + (sizeof(Block)+size+64)) > PEND)
			panic("allocb(%d) called from %#p",
				size, getcallerpc(&size));
		bp = ialloc(sizeof(Block)+size+64, 0);
		addr = (ulong)bp;
		addr = ROUNDUP(addr + sizeof(Block), 8);
		bp->base = (uchar*)addr;
		bp->lim = ((uchar*)bp) + sizeof(Block)+size+64;
	}

	if(bp->flag)
		panic("allocb reuse");

	bp->rp = bp->base;
	bp->wp = bp->rp;
	bp->next = 0;
	bp->flag = 1;

	return bp;
}

void
freeb(Block* bp)
{
	bp->next = allocbp;
	allocbp = bp;

	bp->flag = 0;
}

void (*etherdetach)(void);

void
warp9(ulong entry)
{
	ulong inst;
	ulong *mem;

	if(etherdetach)
		etherdetach();
//	consdrain();
	delay(10);

	splhi();
	trapdisable();
	coherence();
	syncall();

	qtmerrtest("syncs before kernel entry");
	syncall();
	putmsr(getmsr() & ~MSR_ME);	/* disable machine check traps */
	syncall();
	clrmchk();
	syncall();

	mem = (ulong *)PADDR(entry);
	inst = *mem;
	if (inst == 0)
		panic("word at entry addr is zero, kernel not loaded");
	/*
	 * This is where to push things on the stack to
	 * boot *BSD systems, e.g.
	 * (*((void (*)(void*, void*, void*, void*, ulong, ulong))PADDR(entry)))
	 *	(0, 0, 0, 0, 8196, 640);
	 * will enable NetBSD boot (the real memory size needs to
	 * go in the 5th argument).
	 */
	coherence();
	syncall();
	(*(void (*)(void))mem)();

	for (;;)
		;
}
