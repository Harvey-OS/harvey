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

static int x = 0x123456;

/* mach struct for hart 0. */
/* in many plan 9 implementations this stuff is all reserved in early assembly.
 * we don't have to do that. */
static uint64_t m0stack[4096];
static Mach m0;
Sys asys, *sys=&asys;
Conf conf;
uintptr_t kseg0 = KZERO;
char *cputype = "riscv";
int64_t hz;

/* I forget where this comes from and I don't care just now. */
uint32_t kerndate;


/* general purpose hart startup. We call this via startmach.
 * When we enter here, the machp() function is usable.
 */

void hart(void)
{
	//Mach *mach = machp();
	die("not yet");
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
	asminit();
	asmmapinit(0x81000000, 0xc0000000, 1); print("asmmodinit\n");

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
	meminit();print("meminit\n");
	confinit();print("confinit\n");
	archinit();print("archinit\n");
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


	die("Completed hart for bsp OK!\n");
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

void
main(uint32_t mbmagic, uint32_t mbaddress)
{

	testPrint('0');
	if (x != 0x123456)
		die("Data is not set up correctly\n");
	//memset(edata, 0, end - edata);
	//msg("got somewhere");
	startmach(bsp, &m0, m0stack);
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

uintptr_t
userpc(Ureg*u)
{
	panic((char *)__func__);
	return 0;
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

void kexit(Ureg*_)
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

void
setregisters(Ureg*u, char*f, char*t, int amt)
{
	panic((char *)__func__);
}

void cycles(uint64_t *p)
{
	return;
	*p = rdtsc();
}

int islo(void)
{
	msg("isloc\n");
	uint64_t ms = read_csr(sstatus);
	msg("read it\n");
	return ms & MSTATUS_SIE;
}

uintptr_t
dbgpc(Proc*p)
{
	panic((char *)__func__);
	return 0;
}


void dumpstack(void)
{
	panic((char *)__func__);
}

void
dumpgpr(Ureg* ureg)
{
	panic((char *)__func__);
}

void
setkernur(Ureg*u, Proc*p)
{
	panic((char *)__func__);
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

