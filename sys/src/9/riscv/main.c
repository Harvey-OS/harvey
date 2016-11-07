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
#include "io.h"
#include "encoding.h"

extern void (*consuartputs)(char*, int);

void testPrint(uint8_t c);

void msg(char *s)
{
	while (*s)
		testPrint(*s++);
}
void die(char *s)
{
	msg(s);
	while (1);
}

void
ndnr(void)
{
	die("ndnr");
}

static void puts(char * s, int n)
{
	while (n--)
		testPrint(*s++);
}

/* mach info for hart 0. */
/* in many plan 9 implementations this stuff is all reserved in early assembly.
 * we don't have to do that. */
uint64_t m0stack[4096];
Mach m0;

Sys asys, *sys=&asys;
Conf conf;
uintptr_t kseg0 = KZERO;
char *cputype = "riscv";
int64_t hz;

/* I forget where this comes from and I don't care just now. */
uint32_t kerndate;
int maxcores = 1;
int nosmp = 1;

/* general purpose hart startup. We call this via startmach.
 * When we enter here, the machp() function is usable.
 */

void hart(void)
{
	//Mach *mach = machp();
	die("not yet");
}

uint64_t
rdtsc(void)
{
	return read_csr(/*s*/cycle);
}

void
init0(void)
{
	Proc *up = externup();
	char buf[2*KNAMELEN];

	up->nerrlab = 0;

	/*
	 * if(consuart == nil)
	 * i8250console("0");
	 */
	spllo();

	/*
	 * These are o.k. because rootinit is null.
	 * Then early kproc's will have a root and dot.
	 */
	up->slash = namec("#/", Atodir, 0, 0);
	pathclose(up->slash->path);
	up->slash->path = newpath("/");
	up->dot = cclone(up->slash);

	devtabinit();

	if(!waserror()){
		//snprint(buf, sizeof(buf), "%s %s", "AMD64", conffile);
		panic("implement loadenv");
		//loadenv(oargc, oargv);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", cputype, 0);
		ksetenv("pgsz", "2097152", 0);
		// no longer. 	confsetenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);
	//debugtouser((void *)UTZERO);
	panic("fix touser");
	//touser(sp);
}


void
userinit(void)
{
	Proc *up = externup();
	Proc *p;
	Segment *s;
	KMap *k;
	Page *pg;
	int sno;

	p = newproc();
	p->pgrp = newpgrp();
	p->egrp = smalloc(sizeof(Egrp));
	p->egrp->r.ref = 1;
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
	p->sched.sp = PTR2UINT(p->kstack+KSTACK-sizeof(up->arg)-sizeof(uintptr_t));
	p->sched.sp = STACKALIGN(p->sched.sp);

	/*
	 * User Stack
	 *
	 * Technically, newpage can't be called here because it
	 * should only be called when in a user context as it may
	 * try to sleep if there are no pages available, but that
	 * shouldn't be the case here.
	 */
	sno = 0;
	s = newseg(SG_STACK|SG_READ|SG_WRITE, USTKTOP-USTKSIZE, USTKSIZE/ BIGPGSZ);
	p->seg[sno++] = s;
	pg = newpage(1, 0, USTKTOP-BIGPGSZ, BIGPGSZ, -1);
	segpage(s, pg);
	k = kmap(pg);
	panic("fixbootargs");
	//bootargs(VA(k));
	kunmap(k);

	/*
	 * Text
	 */
	s = newseg(SG_TEXT|SG_READ|SG_EXEC, UTZERO, 1);
	s->flushme++;
	p->seg[sno++] = s;
	pg = newpage(1, 0, UTZERO, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	/* UTZERO is only needed until we make init not have 2M block of zeros at the front. */
	memmove(UINT2PTR(VA(k) + init_code_start - UTZERO), init_code_out, sizeof(init_code_out));
	kunmap(k);

	/*
	 * Data
	 */
	s = newseg(SG_DATA|SG_READ|SG_WRITE, UTZERO + BIGPGSZ, 1);
	s->flushme++;
	p->seg[sno++] = s;
	pg = newpage(1, 0, UTZERO + BIGPGSZ, BIGPGSZ, -1);
	memset(pg->cachectl, PG_TXTFLUSH, sizeof(pg->cachectl));
	segpage(s, pg);
	k = kmap(s->map[0]->pages[0]);
	/* This depends on init having a text segment < 2M. */
	memmove(UINT2PTR(VA(k) + init_data_start - (UTZERO + BIGPGSZ)), init_data_out, sizeof(init_data_out));
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


void bsp(void *stack)
{
	Mach *mach = machp();
	if (mach != &m0)
		die("MACH NOT MATCH");
	msg("memset mach\n");
	memset(mach, 0, sizeof(Mach));
	msg("done that\n");

	mach->self = (uintptr_t)mach;
	msg("SET SELF OK\n");
	mach->machno = 0;
	mach->online = 1;
	mach->NIX.nixtype = NIXTC;
	mach->stack = PTR2UINT(stack);
	*(uintptr_t*)mach->stack = STACKGUARD;
	mach->externup = nil;
	active.nonline = 1;
	active.exiting = 0;
	active.nbooting = 0;

	consuartputs = puts;
	msg("call asminit\n");
	msg("==============================================\n");
	asminit();
	msg(",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
	asmmapinit(0x81000000, 0xc0000000, 1); print("asmmapinit\n");

	/*
	 * Need something for initial delays
	 * until a timebase is worked out.
	 */
	mach->cpuhz = 2000000000ll;
	mach->cpumhz = 2000;
	sys->cyclefreq = mach->cpuhz;
	
	sys->nmach = 1;

	fmtinit();
	print("\nHarvey\n");

	mach->perf.period = 1;
	if((hz = archhz()) != 0ll){
		mach->cpuhz = hz;
		mach->cyclefreq = hz;
		sys->cyclefreq = hz;
		mach->cpumhz = hz/1000000ll;
	}

	print("print a number like 5 %d\n", 5);
	/*
	 * Mmuinit before meminit because it
	 * flushes the TLB via machp()->pml4->pa.
	 */
	mmuinit();

	ioinit(); print("ioinit\n");
print("IOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIOIO\n");
	meminit();print("meminit\n");
print("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
	confinit();print("confinit\n");
print("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC\n");
	archinit();print("archinit\n");
print("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
	mallocinit();print("mallocinit\n");

	/* test malloc. It's easier to find out it's broken here,
	 * not deep in some call chain.
	 * See next note.
	 *
	 */
	if (1) {
		void *v = malloc(1234);
		msg("allocated\n ");
		free(v);
		msg("free ok\n");
	}

	umeminit();

	procinit0();
	print("before mpacpi, maxcores %d\n", maxcores);
	trapinit();
	print("trapinit done\n");
	/* Forcing to single core if desired */
	if(!nosmp) {
		// smp startup
	}
//working.
	// not needed. teardownidmap(mach);
	timersinit();print("	timersinit();\n");
	// ? fpuinit();
	psinit(conf.nproc);print("	psinit(conf.nproc);\n");
	initimage();print("	initimage();\n");
	//links();

	devtabreset();print("	devtabreset();\n");
	pageinit();print("	pageinit();\n");
	swapinit();print("	swapinit();\n");
	userinit();print("	userinit();\n");
	/* Forcing to single core if desired */
	if(!nosmp) {
		//nixsquids();
		//testiccs();
	}

	print("NO profiling until you set upa alloc_cpu_buffers()\n");
	//alloc_cpu_buffers();

	print("CPU Freq. %dMHz\n", mach->cpumhz);

	print("schedinit...\n");

	schedinit();
	die("Completed hart for bsp OK!\n");
}

/* stubs until we implement in assembly */
int corecolor(int _)
{
	return -1;
}

Proc *externup(void)
{
	return machp()->externup;
}

void errstr(char *s, int i) {
	panic("errstr");
}

void
oprof_alarm_handler(Ureg *u)
{
	panic((char *)__func__);
}

void
hardhalt(void)
{
	panic((char *)__func__);
}

void
ureg2gdb(Ureg *u, uintptr_t *g)
{
	panic((char *)__func__);
}

int
userureg(Ureg*u)
{
	panic((char *)__func__);
	return -1;
}

void    exit(int _)
{
	panic((char *)__func__);
}

void fpunoted(void)
{
	panic((char *)__func__);
}

void fpunotify(Ureg*_)
{
	panic((char *)__func__);
}

void fpusysrfork(Ureg*_)
{
	panic((char *)__func__);
}

void
reboot(void*_, void*__, int32_t ___)
{
	panic("reboot");
}

void fpusysprocsetup(Proc *_)
{
	panic((char *)__func__);
}

void sysrforkret(void)
{
	panic((char *)__func__);
}

void     fpusysrforkchild(Proc*_, Proc*__)
{
	panic((char *)__func__);
}

int
fpudevprocio(Proc*p, void*v, int32_t _, uintptr_t __, int ___)
{
	panic((char *)__func__);
	return -1;
}

void cycles(uint64_t *p)
{
	return;
	*p = rdtsc();
}

int islo(void)
{
//	msg("isloc\n");
	uint64_t ms = read_csr(sstatus);
//	msg("read it\n");
	return ms & MSTATUS_SIE;
}

void
stacksnippet(void)
{
	//Stackframe *stkfr;
	kmprint(" stack:");
//	for(stkfr = stackframe(); stkfr != nil; stkfr = stkfr->next)
//		kmprint(" %c:%p", ktextaddr(stkfr->pc) ? 'k' : '?', ktextaddr(stkfr->pc) ? (stkfr->pc & 0xfffffff) : stkfr->pc);
	kmprint("\n");
}


/* crap. */
/* this should come from build but it's intimately tied in to VGA. Crap. */
Physseg physseg[8];
int nphysseg = 8;

/* bringup -- remove asap. */
void
DONE(void)
{
	print("DONE\n");
	//prflush();
	delay(10000);
	ndnr();
}

void
HERE(void)
{
	print("here\n");
	//prflush();
	delay(5000);
}

/* The old plan 9 standby ... wave ... */

/* Keep to debug trap.c */
void wave(int c)
{
	testPrint(c);
}

void hi(char *s)
{
	if (! s)
		s = "<NULL>";
	while (*s)
		wave(*s++);
}

