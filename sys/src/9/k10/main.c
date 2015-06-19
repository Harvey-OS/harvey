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

#include "init.h"
#include "apic.h"
#include "io.h"
#include "amd64.h"

#undef DBG
#define DBG iprint

Conf conf;			/* XXX - must go - gag */

static uintptr_t sp;		/* XXX - must go - user stack of init proc */

/* Next time you see a system with cores/sockets running at different clock rates, on x86,
 * let me know. AFAIK, it no longer happens. So the BSP hz is good for the AP hz.
 */
int64_t hz;

uintptr_t kseg0 = KZERO;
Sys* sys = nil;
usize sizeofSys = sizeof(Sys);

Mach *entrym;
/*
 * Option arguments from the command line.
 * oargv[0] is the boot file.
 * Optionsinit() is called from multiboot() to
 * set it all up.
 */
static int64_t oargc;
static char* oargv[20];
static char oargb[128];
static int oargblen;

static int maxcores = 1024;	/* max # of cores given as an argument */
static int numtcs = 32;		/* initial # of TCs */

char dbgflg[256];
static int vflag = 1;

int nosmp = 0;

void
optionsinit(char* s)
{
	oargblen = strecpy(oargb, oargb+sizeof(oargb), s) - oargb;
	oargc = tokenize(oargb, oargv, nelem(oargv)-1);
	oargv[oargc] = nil;
}

static void
options(int argc, char* argv[])
{
	char *p;
	int n, o;

	/*
	 * Process flags.
	 * Flags [A-Za-z] may be optionally followed by
	 * an integer level between 1 and 127 inclusive
	 * (no space between flag and level).
	 * '--' ends flag processing.
	 */
	while(--argc > 0 && (*++argv)[0] == '-' && (*argv)[1] != '-'){
		while(o = *++argv[0]){
			if(!(o >= 'A' && o <= 'Z') && !(o >= 'a' && o <= 'z'))
				continue;
			n = strtol(argv[0]+1, &p, 0);
			if(p == argv[0]+1 || n < 1 || n > 127)
				n = 1;
			argv[0] = p-1;
			dbgflg[o] = n;
		}
	}
	vflag = dbgflg['v'];
	if(argc > 0){
		maxcores = strtol(argv[0], 0, 0);
		argc--;
		argv++;
	}
	if(argc > 0){
		numtcs = strtol(argv[0], 0, 0);
		//argc--;
		//argv++;
	}
	// hack.
	nosmp = dbgflg['n'];
}

void
squidboy(int apicno, Mach *m)
{
	// FIX QEMU. extern int64_t hz;
	int64_t hz;
	sys->machptr[m->machno] = m;
	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = 2000000000ll;
	m->cpumhz = 2000;
	m->perf.period = 1;

	m->nixtype = NIXAC;
	// no NIXAC for now.
	m->nixtype = NIXTC;

	// NOTE: you can't do ANYTHING here before vsvminit.
	// PRINT WILL PANIC. So wait.
	vsvminit(MACHSTKSZ, m->nixtype, m);

	DBG("Hello squidboy %d %d\n", apicno, m->machno);

	/*
	 * Beware the Curse of The Non-Interruptable Were-Temporary.
	 */
	// see comment in main(). This is a 1990s thing. hz = archhz();
	// except qemu won't work unless we do this. Sigh.
	hz = archhz();
	if(hz == 0)
		ndnr();
	// Now most of Phenom and Opterons are right detected.
	else
		ndnr();
	m->cpuhz = hz;
	m->cyclefreq = hz;
	m->cpumhz = hz/1000000ll;

	mmuinit();
	if(!apiconline())
		ndnr();
	fpuinit();

	acmodeset(m->nixtype);
	m->splpc = 0;
	m->online = 1;

	/*
	 * CAUTION: no time sync done, etc.
	 */
	//DBG("Wait for the thunderbirds!\n");
	while(!active.thunderbirdsarego)
		;
	wrmsr(0x10, sys->epoch);
	m->rdtsc = rdtsc();

	print("cpu%d color %d role %s tsc %lld\n",
		m->machno, corecolor(m->machno), rolename[m->nixtype], m->rdtsc);
	switch(m->nixtype){
	case NIXAC:
		acmmuswitch();
		acinit();
		adec(&active.nbooting);
		ainc(&active.nonline);	/* this was commented out */
		acsched();
		panic("squidboy");
		break;
	case NIXTC:
		/*
		 * We only need the idt and syscall entry point actually.
		 * At boot time the boot processor might set our role after
		 * we have decided to become an AC.
		 */
		vsvminit(MACHSTKSZ, NIXTC, m);

		/*
		 * Enable the timer interrupt.
		 */
		apictimerenab();
		apicpri(0);

		timersinit();
		adec(&active.nbooting);
		ainc(&active.nonline);

		schedinit();
		break;
	}
	panic("squidboy returns (type %d)", m->nixtype);
}

static void
testiccs(void)
{
	int i;
	Mach *mp;
	extern void testicc(int);

	/* setup arguments for all */
	for(i = 0; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online && mp->nixtype == NIXAC)
			testicc(i);
	print("bootcore: all cores done\n");
}

/*
 * Rendezvous with other cores. Set roles for those that came
 * up online, and wait until they are initialized.
 * Sync TSC with them.
 * We assume other processors that could boot had time to
 * set online to 1 by now.
 */
static void
nixsquids(void)
{
	Mach *m = machp();
	Mach *mp;
	int i;
	uint64_t now, start;

	for(i = 1; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online){
			/*
			 * Inter-core calls. A ensure *mp->iccall and mp->icargs
			 * go into different cache lines.
			 */
			mp->icc = mallocalign(sizeof *m->icc, ICCLNSZ, 0, 0);
			mp->icc->fn = nil;
			if(i < numtcs){
				sys->nmach++;
				mp->nixtype = NIXTC;
				sys->nc[NIXTC]++;
			}else
				sys->nc[NIXAC]++;
			ainc(&active.nbooting);
		}
	sys->epoch = rdtsc();
	mfence();
	wrmsr(0x10, sys->epoch);
	m->rdtsc = rdtsc();
	active.thunderbirdsarego = 1;
	start = fastticks2us(fastticks(nil));
	do{
		now = fastticks2us(fastticks(nil));
	}while(active.nbooting > 0 && now - start < 1000000)
		;
	if(active.nbooting > 0)
		print("cpu0: %d cores couldn't start\n", active.nbooting);
	active.nbooting = 0;
}

void
DONE(void)
{
	print("DONE\n");
	prflush();
	delay(10000);
	ndnr();
}

void
HERE(void)
{
	print("here\n");
	prflush();
	delay(5000);
}

/* The old plan 9 standby ... wave ... */

/* Keep to debug trap.c */
void wave(int c)
{
	outb(0x3f8, c);
}

void hi(char *s) 
{
	if (! s)
		s = "<NULL>";
	while (*s)
		wave(*s++);
}

/*
 * for gdb: 
 * call this anywhere in your code. 
 *   die("yourturn with gdb\n");
 *   gdb 9k
 *   target remote localhost:1234
 *   display/i $pc
 *   set staydead = 0
 *   stepi, and debug. 
 * note, you can always resume after a die. Just set staydead = 0
 */

int staydead = 1;
void die(char *s)
{
	wave('d');
	wave('i');
	wave('e');
	wave(':');
	hi(s);
	while(staydead);
	staydead = 1;
}

/*
void bmemset(void *p)
{
	__asm__ __volatile__("1: jmp 1b");
}

void put8(uint8_t c)
{
	char x[] = "0123456789abcdef";
	wave(x[c>>4]);
	wave(x[c&0xf]);
}

void put16(uint16_t s)
{
	put8(s>>8);
	put8(s);
}

void put32(uint32_t u)
{
	put16(u>>16);
	put16(u);
}

void put64(uint64_t v)
{
	put32(v>>32);
	put32(v);
}
*/

void debugtouser(void *va)
{
	Mach *m = machp();
	uintptr_t uva = (uintptr_t) va;
	PTE *pte, *pml4;

	pml4 = UINT2PTR(m->pml4->va);
	mmuwalk(pml4, uva, 0, &pte, nil);
	iprint("va %p m %p m>pml4 %p m->pml4->va %p pml4 %p PTE 0x%lx\n", va,
			m, m->pml4, m->pml4->va, (void *)pml4, *pte);
}

/*
void badcall(uint64_t where, uint64_t what)
{
	hi("Bad call from function "); put64(where); hi(" to "); put64(what); hi("\n");
	while (1)
		;
}
*/

void errstr(char *s, int i) {
	panic("errstr");
}

static int x = 0x123456;

/* tear down the identity map we created in assembly. ONLY do this after all the
 * APs have started up (and you know they've done so. But you must do it BEFORE
 * you create address spaces for procs, i.e. userinit()
 */
static void
teardownidmap(Mach *m)
{
	int i;
	uintptr_t va = 0;
	PTE *p;
	/* loop on the level 2 because we should not assume we know
	 * how many there are But stop after 1G no matter what, and
	 * report if there were that many, as that is odd.
	 */
	for(i = 0; i < 512; i++, va += BIGPGSZ) {
		if (mmuwalk(UINT2PTR(m->pml4->va), va, 1, &p, nil) != 1)
			break;
		if (! *p)
			break;
		iprint("teardown: va %p, pte %p\n", (void *)va, p);
		*p = 0;
	}
	iprint("Teardown: zapped %d PML1 entries\n", i);

	for(i = 2; i < 4; i++) {
		if (mmuwalk(UINT2PTR(m->pml4->va), 0, i, &p, nil) != i) {
			iprint("weird; 0 not mapped at %d\n", i);
			continue;
		}
		iprint("teardown: zap %p at level %d\n", p, i);
		if (p)
			*p = 0;
	}
}


void
main(uint32_t mbmagic, uint32_t mbaddress)
{
	Mach *m = entrym;
	/* when we get here, entrym is set to core0 mach. */
	sys->machptr[m->machno] = m;
	// Very special case for BSP only. Too many things
	// assume this is set.
	wrmsr(GSbase, PTR2UINT(&sys->machptr[m->machno]));
	if (machp() != m)
		panic("m and machp() are different!!\n");
	assert(sizeof(Mach) <= PGSZ);

	/*
	 * Check that our data is on the right boundaries.
	 * This works because the immediate value is in code.
	 */
	if (x != 0x123456) 
		panic("Data is not set up correctly\n");
	memset(edata, 0, end - edata);

	m = (void *) (KZERO + 1048576 + 11*4096);
	sys = (void *) (KZERO + 1048576);

	/*
	 * ilock via i8250enable via i8250console
	 * needs m->machno, sys->machptr[] set, and
	 * also 'up' set to nil.
	 */
	cgapost(sizeof(uintptr_t)*8);
	memset(m, 0, sizeof(Mach));

	m->machno = 0;
	m->online = 1;
	m->nixtype = NIXTC;
	sys->machptr[m->machno] = &sys->mach;
	m->stack = PTR2UINT(sys->machstk);
	m->vsvm = sys->vsvmpage;
	m->externup = (void *)0;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	asminit();
	multiboot(mbmagic, mbaddress, 0);
	options(oargc, oargv);

	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	m->cpuhz = 2000000000ll;
	m->cpumhz = 2000;

	cgainit();
	i8250console("0");
	
	consputs = cgaconsputs;

	/* It all ends here. */
	vsvminit(MACHSTKSZ, NIXTC, m);
	if (machp() != m)
		panic("After vsvminit, m and machp() are different");
	fmtinit();
	
	print("\nHarvey\n");
	sys->nmach = 1;			

	if(1){
		multiboot(mbmagic, mbaddress, vflag);
	}

	m->perf.period = 1;
	if((hz = archhz()) != 0ll){
		m->cpuhz = hz;
		m->cyclefreq = hz;
		m->cpumhz = hz/1000000ll;
	}
	//iprint("archhz returns 0x%lld\n", hz);
	iprint("NOTE: if cpuidhz runs too fast, we get die early with a NULL pointer\n");
	iprint("So, until that's fixed, we bring up AP cores slowly. Sorry!\n");

	/*
	 * Mmuinit before meminit because it
	 * flushes the TLB via m->pml4->pa.
	 */
	mmuinit();
	ioinit();
	kbdinit();
	meminit();
	confinit();
	archinit();
	mallocinit();

	/* test malloc. It's easier to find out it's broken here, 
	 * not deep in some call chain.
	 * See next note. 
	 *
	void *v = malloc(1234);
	hi("v "); put64((uint64_t)v); hi("\n");
	free(v);
	hi("free ok\n");
	 */

	/*
	 * Acpiinit will cause the first malloc
	 * call to happen.
	 * If the system dies here it's probably due
	 * to malloc not being initialised
	 * correctly, or the data segment is misaligned
	 * (it's amazing how far you can get with
	 * things like that completely broken).
	 */
if (0){	acpiinit(); hi("	acpiinit();\n");}
	
	umeminit();
	trapinit();
	printinit();

	/*
	 * This is necessary with GRUB and QEMU.
	 * Without it an interrupt can occur at a weird vector,
	 * because the vector base is likely different, causing
	 * havoc. Do it before any APIC initialisation.
	 */
	i8259init(32);

	procinit0();
	mpsinit(maxcores);
	apiconline();
	/* Forcing to single core if desired */
	if(!nosmp) {
		sipi();
	}
	teardownidmap(m);
	timersinit();
	kbdenable();
	fpuinit();
	psinit(conf.nproc);
	initimage();
	links();
	devtabreset();
	pageinit();
	swapinit();
	userinit();
	/* Forcing to single core if desired */
	if(!nosmp) {
		nixsquids();
		testiccs();
	}

	print("CPU Freq. %dMHz\n", m->cpumhz);

	print("schedinit...\n");
	schedinit();
}

void
init0(void)
{
	Mach *m = machp();
	char buf[2*KNAMELEN];

	m->externup->nerrlab = 0;

	/*
	 * if(consuart == nil)
	 * i8250console("0");
	 */
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	m->externup->slash = namec("#/", Atodir, 0, 0);
	pathclose(m->externup->slash->path);
	m->externup->slash->path = newpath("/");
	m->externup->dot = cclone(m->externup->slash);

	devtabinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", "AMD64", conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		ksetenv("pgsz", "2097152", 0);
		// no longer. 	confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	debugtouser((void *)UTZERO);
	touser(sp);
}

void
bootargs(uintptr_t base)
{
	int i;
	uint32_t ssize;
	char **av, *p;

	/*
	 * Push the boot args onto the stack.
	 * Make sure the validaddr check in syscall won't fail
	 * because there are fewer than the maximum number of
	 * args by subtracting sizeof(m->externup->arg).
	 */
	i = oargblen+1;
	p = UINT2PTR(STACKALIGN(base + BIGPGSZ - sizeof(entrym->externup->arg) - i));
	memmove(p, oargb, i);

	/*
	 * Now push argc and the argv pointers.
	 * This isn't strictly correct as the code jumped to by
	 * touser in init9.[cs] calls startboot (port/initcode.c) which
	 * expects arguments
	 * 	startboot(char* argv0, char* argv[])
	 * not the usual (int argc, char* argv[]), but argv0 is
	 * unused so it doesn't matter (at the moment...).
	 */
	av = (char**)(p - (oargc+2)*sizeof(char*));
	ssize = base + BIGPGSZ - PTR2UINT(av);
	*av++ = (char*)oargc;
	for(i = 0; i < oargc; i++)
		*av++ = (oargv[i] - oargb) + (p - base) + (USTKTOP - BIGPGSZ);
	*av = nil;

	sp = USTKTOP - ssize;
}

void
userinit(void)
{
	Mach *m = machp();
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;

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
	 *
	 * N.B. make sure there's enough space for syscall to check
	 *	for valid args and
	 *	space for gotolabel's return PC
	 * AMD64 stack must be quad-aligned.
	 */
	p->sched.pc = PTR2UINT(init0);
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(m->externup->arg)-sizeof(uintptr_t));
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	s = newseg(SG_STACK, USTKTOP-USTKSIZE, USTKSIZE/ BIGPGSZ);
	p->seg[SSEG] = s;
	pg = newpage(1, 0, USTKTOP-BIGPGSZ, BIGPGSZ, -1);
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
	pg = newpage(1, 0, UTZERO, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	//memmove(UINT2PTR(VA(k)), initcode, sizeof(initcode));
	memmove(UINT2PTR(VA(k)), init_code_out, sizeof(init_code_out));
	kunmap(k);

	/*
	 * Data
	 */
	s = newseg(SG_DATA, UTZERO + BIGPGSZ, 1);
	s->flushme++;
	p->seg[DSEG] = s;
	pg = newpage(1, 0, UTZERO + BIGPGSZ, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	//memmove(UINT2PTR(VA(k)), initcode, sizeof(initcode));
	memmove(UINT2PTR(VA(k)), init_data_out, sizeof(init_data_out));
	kunmap(k);
	ready(p);
}

void
confinit(void)
{
	int i;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
		conf.npage += conf.mem[i].npage;
	conf.nproc = 1000;
	conf.nimage = 200;
}

static void
shutdown(int ispanic)
{
	Mach *m = machp();
	int ms, once;

	lock(&active);
	if(ispanic)
		active.ispanic = ispanic;
	else if(m->machno == 0 && m->online == 0)
		active.ispanic = 0;
	once = m->online;
	m->online = 0;
	adec(&active.nonline);
	active.exiting = 1;
	unlock(&active);

	if(once)
		iprint("cpu%d: exiting\n", m->machno);

	spllo();
	for(ms = 5*1000; ms > 0; ms -= TK2MS(2)){
		delay(TK2MS(2));
		if(active.nonline == 0 && consactive() == 0)
			break;
	}

	if(active.ispanic && m->machno == 0){
		if(cpuserver)
			delay(30000);
		else
			for(;;)
				halt();
	}
	else
		delay(1000);
}

void
reboot(void* v, void* w, int32_t i)
{
	panic("reboot\n");
}

void
exit(int ispanic)
{
	shutdown(ispanic);
	archreset();
}
