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

#include <tos.h>
#include <pool.h>
#include "amd64.h"
#include "ureg.h"
#include "io.h"
#include "../port/pmc.h"

/*
 * NIX code run at the AC.
 * This is the "AC kernel".
 */

/*
 * FPU:
 *
 * The TC handles the FPU by keeping track of the state for the
 * current process. If it has been used and must be saved, it is saved, etc.
 * When a process gets to the AC, we handle the FPU directly, and save its
 * state before going back to the TC (or the TC state would be stale).
 *
 * Because of this, each time the process comes back to the AC and
 * uses the FPU it will get a device not available trap and
 * the state will be restored. This could be optimized because the AC
 * is single-process, and we do not have to disable the FPU while
 * saving, so it does not have to be restored.
 */

extern char* acfpunm(Ureg* ureg, void*);
extern char* acfpumf(Ureg* ureg, void*);
extern char* acfpuxf(Ureg* ureg, void*);
extern void acfpusysprocsetup(Proc*);

extern void _acsysret(void);
extern void _actrapret(void);

ACVctl *acvctl[256];

/* 
 * Test inter core calls by calling a cores to print something, and then
 * waiting for it to complete.
 */
static void
testiccfn(void)
{
	print("called: %s\n", (char*)m->icc->data);
}

void
testicc(int i)
{
	Mach *mp;

	if((mp = sys->machptr[i]) != nil && mp->online != 0){
		if(mp->nixtype != NIXAC){
			print("testicc: core %d is not an AC\n", i);
			return;
		}
		print("calling core %d... ", i);
		mp->icc->flushtlb = 0;
		snprint((char*)mp->icc->data, ICCLNSZ, "<%d>", i);
		mfence();
		mp->icc->fn = testiccfn;
		mwait(&mp->icc->fn);
	}
}

/*
 * Check if the AC kernel (mach) stack has more than 4*KiB free.
 * Do not call panic, the stack is gigantic.
 */
static void
acstackok(void)
{
	char dummy;
	char *sstart;

	sstart = (char *)m - PGSZ - 4*PTSZ - MACHSTKSZ;
	if(&dummy < sstart + 4*KiB){
		print("ac kernel stack overflow, cpu%d stopped\n", m->machno);
		DONE();
	}
}

/*
 * Main scheduling loop done by the application core.
 * Some of functions run will not return.
 * The system call handler will reset the stack and
 * call acsched again.
 * We loop because some functions may return and we should
 * wait for another call.
 */
void
acsched(void)
{
	acmmuswitch();
	for(;;){
		acstackok();
		mwait(&m->icc->fn);
		if(m->icc->flushtlb)
			acmmuswitch();
		DBG("acsched: cpu%d: fn %#p\n", m->machno, m->icc->fn);
		m->icc->fn();
		DBG("acsched: cpu%d: idle\n", m->machno);
		mfence();
		m->icc->fn = nil;
	}
}

void
acmmuswitch(void)
{
	extern Page mach0pml4;

	DBG("acmmuswitch mpl4 %#p mach0pml4 %#p m0pml4 %#p\n", m->pml4->pa, mach0pml4.pa, sys->machptr[0]->pml4->pa);


	cr3put(m->pml4->pa);
}

/*
 * Beware: up is not set when this function is called.
 */
void
actouser(void)
{
	void xactouser(u64int);
	Ureg *u;

	acfpusysprocsetup(m->proc);

	u = m->proc->dbgreg;
	DBG("cpu%d: touser usp = %#p entry %#p\n", m->machno, u->sp, u->ip);
	xactouser(u->sp);
	panic("actouser");
}

void
actrapret(void)
{
	/* done by actrap() */
}

/*
 * Entered in AP core context, upon traps (system calls go through acsyscall)
 * using up->dbgreg means cores MUST be homogeneous.
 *
 * BUG: We should setup some trapenable() mechanism for the AC,
 * so that code like fpu.c could arrange for handlers specific for
 * the AC, instead of doint that by hand here.
 * 
 * All interrupts are masked while in the "kernel"
 */
void
actrap(Ureg *u)
{
	char *n;
	ACVctl *v;

	n = nil;

	_pmcupdate(m);
	if(m->proc != nil){
		m->proc->nactrap++;
		m->proc->actime1 = fastticks(nil);
	}
	if(u->type < nelem(acvctl)){
		v = acvctl[u->type];
		if(v != nil){
			DBG("actrap: cpu%d: %ulld\n", m->machno, u->type);
			n = v->f(u, v->a);
			if(n != nil)
				goto Post;
			return;
		}
	}
	switch(u->type){
	case IdtDF:
		print("AC: double fault\n");
		dumpregs(u);
		ndnr();
	case IdtIPI:
		m->intr++;
		DBG("actrap: cpu%d: IPI\n", m->machno);
		apiceoi(IdtIPI);
		break;
	case IdtTIMER:
		apiceoi(IdtTIMER);
		panic("timer interrupt in an AC");
		break;
	case IdtPF:
		/* this case is here for debug only */
		m->pfault++;
		DBG("actrap: cpu%d: PF cr2 %#ullx\n", m->machno, cr2get());
		break;
	default:
		print("actrap: cpu%d: %ulld\n", m->machno, u->type);
	}
Post:
	m->icc->rc = ICCTRAP;
	m->cr2 = cr2get();
	memmove(m->proc->dbgreg, u, sizeof *u);
	m->icc->note = n;
	fpuprocsave(m->proc);
	_pmcupdate(m);
	mfence();
	m->icc->fn = nil;
	ready(m->proc);

	mwait(&m->icc->fn);

	if(m->icc->flushtlb)
		acmmuswitch();
	if(m->icc->fn != actrapret)
		acsched();
	DBG("actrap: ret\n");
	memmove(u, m->proc->dbgreg, sizeof *u);
	if(m->proc)
		m->proc->actime += fastticks2us(fastticks(nil) - m->proc->actime1);
}

void
acsyscall(void)
{
	Proc *p;

	/*
	 * If we saved the Ureg into m->proc->dbgregs,
	 * There's nothing else we have to do.
	 * Otherwise, we should m->proc->dbgregs = u;
	 */
	DBG("acsyscall: cpu%d\n", m->machno);

	_pmcupdate(m);
	p = m->proc;
	p->actime1 = fastticks(nil);
	m->syscall++;	/* would also count it in the TS core */
	m->icc->rc = ICCSYSCALL;
	m->cr2 = cr2get();
	fpuprocsave(p);
	_pmcupdate(m);
	mfence();
	m->icc->fn = nil;
	ready(p);
	/*
	 * The next call is probably going to make us jmp
	 * into user code, forgetting all our state in this
	 * stack, upon the next syscall.
	 * We don't nest calls in the current stack for too long.
	 */
	acsched();
}

/*
 * Called in AP core context, to return from system call.
 */
void
acsysret(void)
{
	DBG("acsysret\n");
	if(m->proc != nil)
		m->proc->actime += fastticks2us(fastticks(nil) - m->proc->actime1);
	_acsysret();
}

void
dumpreg(void *u)
{
	print("reg is %p\n", u);
	ndnr();
}

char *rolename[] = 
{
	[NIXAC]	"AC",
	[NIXTC]	"TC",
	[NIXKC]	"KC",
	[NIXXC]	"XC",
};

void
acmodeset(int mode)
{
	switch(mode){
	case NIXAC:
	case NIXKC:
	case NIXTC:
	case NIXXC:
		break;
	default:
		panic("acmodeset: bad mode %d", mode);
	}
	m->nixtype = mode;
}

void
acinit(void)
{
	Mach *mp;
	Proc *pp;

	/*
	 * Lower the priority of the apic to 0,
	 * to accept interrupts.
	 * Raise it later if needed to disable them.
	 */
	apicpri(0);

	/*
	 * Be sure a few  assembler assumptions still hold.
	 * Someone moved m->stack and I had fun debugging...
	 */
	mp = 0;
	pp = 0;
	assert((uintptr)&mp->proc == 16);
	assert((uintptr)&pp->dbgreg == 24);
	assert((uintptr)&mp->stack == 24);
}
