#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "init.h"
#include "arm.h"
#include <pool.h>

#include "reboot.h"

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 * There are 3584 bytes available at CONFADDR.
 */
#define BOOTARGS	((char*)CONFADDR)
#define	BOOTARGSLEN	(16*KiB)		/* limit in devenv.c */
#define	MAXCONF		64
#define MAXCONFLINE	160

enum {
	Maxmem	= 512*MB,			/* limited by address ranges */
	Minmem	= 256*MB,			/* conservative default */
};

#define isascii(c) ((uchar)(c) > 0 && (uchar)(c) < 0177)

uintptr kseg0 = KZERO;
Mach* machaddr[MAXMACH];

/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot()
 * or some other machine-dependent place
 * to set it all up.
 */
static int oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;
static char oenv[4096];

static uintptr sp;		/* XXX - must go - user stack of init proc */

int vflag;
char debug[256];

/* store plan9.ini contents here at least until we stash them in #ec */
static char confname[MAXCONF][KNAMELEN];
static char confval[MAXCONF][MAXCONFLINE];
static int nconf;

#ifdef CRYPTOSANDBOX
uchar sandbox[64*1024+BY2PG];
#endif

static int
findconf(char *name)
{
	int i;

	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			return i;
	return -1;
}

char*
getconf(char *name)
{
	int i;

	i = findconf(name);
	if(i >= 0)
		return confval[i];
	return nil;
}

void
addconf(char *name, char *val)
{
	int i;

	i = findconf(name);
	if(i < 0){
		if(val == nil || nconf >= MAXCONF)
			return;
		i = nconf++;
		strecpy(confname[i], confname[i]+sizeof(confname[i]), name);
	}
//	confval[i] = val;
	strecpy(confval[i], confval[i]+sizeof(confval[i]), val);
}

static void
writeconf(void)
{
	char *p, *q;
	int n;

	p = getconfenv();

	if(waserror()) {
		free(p);
		nexterror();
	}

	/* convert to name=value\n format */
	for(q=p; *q; q++) {
		q += strlen(q);
		*q = '=';
		q += strlen(q);
		*q = '\n';
	}
	n = q - p + 1;
	if(n >= BOOTARGSLEN)
		error("kernel configuration too large");
	memmove(BOOTARGS, p, n);
	poperror();
	free(p);
}

/*
 * assumes that we have loaded our /cfg/pxe/mac file at 0x1000 with
 * tftp in u-boot.  no longer uses malloc, so can be called early.
 */
static void
plan9iniinit(void)
{
	char *k, *v, *next;

	k = (char *)CONFADDR;
	if(!isascii(*k))
		return;

	for(; k && *k != '\0'; k = next) {
		if (!isascii(*k))		/* sanity check */
			break;
		next = strchr(k, '\n');
		if (next)
			*next++ = '\0';

		if (*k == '\0' || *k == '\n' || *k == '#')
			continue;
		v = strchr(k, '=');
		if(v == nil)
			continue;		/* mal-formed line */
		*v++ = '\0';

		addconf(k, v);
	}
}

static void
optionsinit(char* s)
{
	char *o;

	o = strecpy(oargb, oargb+sizeof(oargb), s)+1;
	if(getenv("bootargs", o, o - oargb) != nil)
		*(o-1) = ' ';

	oargblen = strlen(oargb);
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

char*
getenv(char* name, char* buf, int n)
{
	char *e, *p, *q;

	p = oenv;
	while(*p != 0){
		if((e = strchr(p, '=')) == nil)
			break;
		for(q = name; p < e; p++){
			if(*p != *q)
				break;
			q++;
		}
		if(p == e && *q == 0){
			strecpy(buf, buf+n, e+1);
			return buf;
		}
		p += strlen(p)+1;
	}

	return nil;
}

#include "io.h"

typedef struct Spiregs Spiregs;
struct Spiregs {
	ulong	ictl;		/* interface ctl */
	ulong	icfg;		/* interface config */
	ulong	out;		/* data out */
	ulong	in;		/* data in */
	ulong	ic;		/* interrupt cause */
	ulong	im;		/* interrupt mask */
	ulong	_pad[2];
	ulong	dwrcfg;		/* direct write config */
	ulong	dwrhdr;		/* direct write header */
};

enum {
	/* ictl bits */
	Csnact	= 1<<0,		/* serial memory activated */

	/* icfg bits */
	Bytelen	= 1<<5,		/* 2^(this_bit) bytes per transfer */
	Dirrdcmd= 1<<10,	/* flag: fast read */
};

static void
dumpbytes(uchar *bp, long max)
{
	iprint("%#p: ", bp);
	for (; max > 0; max--)
		iprint("%02.2ux ", *bp++);
	iprint("...\n");
}

void	archconsole(void);
vlong	probeaddr(uintptr);

static void
spiprobe(void)
{
if (0) {
/* generates repeated "spurious irqbridge interrupt: 00000010" on sheevaplug. */
	Spiregs *rp = (Spiregs *)soc.spi;

	if (probeaddr(soc.spi) < 0)
		return;
	rp->ictl |= Csnact;
	coherence();
	rp->icfg |= Dirrdcmd | 3<<8;	/* fast reads, 4-byte addresses */
	rp->icfg &= ~Bytelen;		/* one-byte reads */
	coherence();

	print("spi flash ignored: ctlr %#p, data %#ux", rp, PHYSSPIFLASH);
	mmuidmap(PHYSSPIFLASH, 1);
	if (probeaddr(PHYSSPIFLASH) < 0)
		print(" (no response)");
	print(": memory reads enabled\n");
}
}

/*
 * entered from l.s with mmu enabled.
 *
 * we may have to realign the data segment; apparently 5l -H0 -R4096
 * does not pad the text segment.  on the other hand, we may have been
 * loaded by another kernel.
 *
 * be careful not to touch the data segment until we know it's aligned.
 */
void
main(Mach* mach)
{
	extern char bdata[], edata[], end[], etext[];
	static ulong vfy = 0xcafebabe;

	m = mach;
	if (vfy != 0xcafebabe)
		memmove(bdata, etext, edata - bdata);
	if (vfy != 0xcafebabe) {
		wave('?');
		panic("misaligned data segment");
	}
	memset(edata, 0, end - edata);		/* zero bss */
	vfy = 0;

wave('9');
	machinit();
	archreset();
	mmuinit();

	optionsinit("/boot/boot boot");
	quotefmtinstall();
	archconsole();
wave(' ');

	/* want plan9.ini to be able to affect memory sizing in confinit */
	plan9iniinit();		/* before we step on plan9.ini in low memory */

	/* set memsize before xinit */
	confinit();
	/* xinit would print if it could */
	xinit();

	/*
	 * Printinit will cause the first malloc call.
	 * (printinit->qopen->malloc) unless any of the
	 * above (like clockintr) do an irqenable, which
	 * will call malloc.
	 * If the system dies here it's probably due
	 * to malloc(->xalloc) not being initialised
	 * correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with
	 * things like that completely broken).
	 *
	 * (Should be) boilerplate from here on.
	 */
	trapinit();
	clockinit();

	printinit();
	uartkirkwoodconsole();
	/* only now can we print */
	print("from Bell Labs\n\n");

#ifdef CRYPTOSANDBOX
	print("sandbox: 64K at physical %#lux, mapped to 0xf10b0000\n",
		PADDR((uintptr)sandbox & ~(BY2PG-1)));
#endif

	archconfinit();
	cpuidprint();
	timersinit();

	procinit0();
	initseg();
	links();
	chandevreset();			/* most devices are discovered here */
	spiprobe();

	pageinit();
	swapinit();
	userinit();
	schedinit();
	panic("schedinit returned");
}

void
cpuidprint(void)
{
	char name[64];

	cputype2name(name, sizeof name);
	print("cpu%d: %lldMHz ARM %s\n", m->machno, m->cpuhz/1000000, name);
}

void
machinit(void)
{
	memset(m, 0, sizeof(Mach));
	m->machno = 0;
	machaddr[m->machno] = m;

	m->ticks = 1;
	m->perf.period = 1;

	conf.nmach = 1;

	active.machs = 1;
	active.exiting = 0;

	up = nil;
}

static void
shutdown(int ispanic)
{
	int ms, once;

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && (active.machs & (1<<m->machno)) == 0)
		active.ispanic = 0;
	once = active.machs & (1<<m->machno);
	active.machs &= ~(1<<m->machno);
	active.exiting = 1;
	unlock(&active);

	if(once)
		iprint("cpu%d: exiting\n", m->machno);
	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.machs == 0 && consactive() == 0)
			break;
	}
	delay(1000);
}

/*
 *  exit kernel either on a panic or user request
 */
void
exit(int code)
{
	shutdown(code);
	splhi();
	archreboot();
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and entry point `entry'.
 */
void
reboot(void *entry, void *code, ulong size)
{
	void (*f)(ulong, ulong, ulong);

	iprint("starting reboot...");
	writeconf();
	
	shutdown(0);

	/*
	 * should be the only processor running now
	 */

	print("shutting down...\n");
	delay(200);

	/* turn off buffered serial console */
	serialoq = nil;

	/* shutdown devices */
	chandevshutdown();

	/* call off the dog */
	clockshutdown();

	splhi();

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));
	cacheuwbinv();
	l2cacheuwb();

	print("rebooting...");
	iprint("entry %#lux code %#lux size %ld\n",
		PADDR(entry), PADDR(code), size);
	delay(100);		/* wait for uart to quiesce */

	/* off we go - never to return */
	cacheuwbinv();
	l2cacheuwb();
	(*f)(PADDR(entry), PADDR(code), size);

	iprint("loaded kernel returned!\n");
	delay(1000);
	archreboot();
}

/*
 *  starting place for first process
 */
void
init0(void)
{
	int i;
	char buf[2*KNAMELEN];

	assert(up != nil);
	up->nerrlab = 0;
	coherence();
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	chandevinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", "ARM", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "arm", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);

		/* convert plan9.ini variables to #e and #ec */
		for(i = 0; i < nconf; i++) {
			ksetenv(confname[i], confval[i], 0);
			ksetenv(confname[i], confval[i], 1);
		}
		poperror();
	}
	kproc("alarm", alarmkproc, 0);

	touser(sp);
}

static void
bootargs(uintptr base)
{
	int i;
	ulong ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * The initial value of the user stack must be such
	 * that the total used is larger than the maximum size
	 * of the argument list checked in syscall.
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + BY2PG - sizeof(up->s.args) - i));
	memmove(p, oargb, i);

	/*
	 * Now push argc and the argv pointers.
	 * This isn't strictly correct as the code jumped to by
	 * touser in init9.s calls startboot (port/initcode.c) which
	 * expects arguments
	 * 	startboot(char *argv0, char **argv)
	 * not the usual (int argc, char* argv[]), but argv0 is
	 * unused so it doesn't matter (at the moment...).
	 */
	av = (char**)(p - (oargc+2)*sizeof(char*));
	ssize = base + BY2PG - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BY2PG);
	*av = nil;

	/*
	 * Leave space for the return PC of the
	 * caller of initcode.
	 */
	sp = USTKTOP - ssize - sizeof(void*);
}

/*
 *  create the first process
 */
void
userinit(void)
{
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;

	/* no processes yet */
	up = nil;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->ref = 1;
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->procmode = 0640;

	kstrdup(&eve, "");
	kstrdup(&p->text, "*init*");
	kstrdup(&p->user, eve);

	/*
	 * Kernel Stack
	 */
	p->sched.pc = PTR2UINT(init0);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->s.args)-sizeof(uintptr));
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/BY2PG);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BY2PG);
	segpage(s, pg);
	k = kmap(pg);
	bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT, UTZERO, 1);
	s->flushme++;
	p->seg[TSEG] = s;
	pg = newpage(1, 0, UTZERO);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	memmove(UINT2PTR(VA(k)), initcode, sizeof initcode);
	kunmap(k);

	ready(p);
}

Conf conf;			/* XXX - must go - gag */

Confmem sheevamem[nelem(conf.mem)] = {
	/*
	 * Memory available to Plan 9:
	 * the 8K is reserved for ethernet dma access violations to scribble on.
	 */
	{ .base = PHYSDRAM, .limit = PHYSDRAM + Maxmem - 8*1024, },
};
ulong memsize = Maxmem;

static int
gotmem(uintptr sz)
{
	uintptr addr;

	addr = PHYSDRAM + sz - MB;
	mmuidmap(addr, 1);
	if (probeaddr(addr) >= 0) {
		memsize = sz;
		return 0;
	}
	return -1;
}

void
confinit(void)
{
	int i;
	ulong kpages;
	uintptr pa;
	char *p;

	/*
	 * Copy the physical memory configuration to Conf.mem.
	 */
	if(nelem(sheevamem) > nelem(conf.mem)){
		iprint("memory configuration botch\n");
		exit(1);
	}
	if((p = getconf("*maxmem")) != nil) {
		memsize = strtoul(p, 0, 0) - PHYSDRAM;
		if (memsize < 16*MB)		/* sanity */
			memsize = 16*MB;
	}

	/*
	 * see if all that memory exists; if not, find out how much does.
	 * trapinit must have been called first.
	 */
	if (gotmem(memsize) < 0 && gotmem(256*MB) < 0 && gotmem(128*MB) < 0) {
		iprint("can't find any memory, assuming %dMB\n", Minmem / MB);
		memsize = Minmem;
	}

	sheevamem[0].limit = PHYSDRAM + memsize - 8*1024;
	memmove(conf.mem, sheevamem, sizeof(sheevamem));

	conf.npage = 0;
	pa = PADDR(PGROUND(PTR2UINT(end)));

	/*
	 *  we assume that the kernel is at the beginning of one of the
	 *  contiguous chunks of memory and fits therein.
	 */
	for(i=0; i<nelem(conf.mem); i++){
		/* take kernel out of allocatable space */
		if(pa > conf.mem[i].base && pa < conf.mem[i].limit)
			conf.mem[i].base = pa;

		conf.mem[i].npage = (conf.mem[i].limit - conf.mem[i].base)/BY2PG;
		conf.npage += conf.mem[i].npage;
	}

	conf.upages = (conf.npage*90)/100;
	conf.ialloc = ((conf.npage-conf.upages)/2)*BY2PG;

	/* only one processor */
	conf.nmach = 1;

	/* set up other configuration parameters */
	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	if(cpuserver)
		conf.nproc *= 3;
	if(conf.nproc > 2000)
		conf.nproc = 2000;
	conf.nswap = conf.npage*3;
	conf.nswppo = 4096;
	conf.nimage = 200;

	conf.copymode = 0;		/* copy on write */

	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for
	 * (probably ~300KB).
	 */
	kpages = conf.npage - conf.upages;
	kpages *= BY2PG;
	kpages -= conf.upages*sizeof(Page)
		+ conf.nproc*sizeof(Proc)
		+ conf.nimage*sizeof(Image)
		+ conf.nswap
		+ conf.nswppo*sizeof(Page);
	mainmem->maxsize = kpages;
	if(!cpuserver)
		/*
		 * give terminals lots of image memory, too; the dynamic
		 * allocation will balance the load properly, hopefully.
		 * be careful with 32-bit overflow.
		 */
		imagmem->maxsize = kpages;
}

int
cmpswap(long *addr, long old, long new)
{
	return cas32(addr, old, new);
}
