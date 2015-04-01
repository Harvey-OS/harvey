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

Lock nixaclock;	/* NIX AC lock; held while assigning procs to cores */

/*
 * NIX support for the time sharing core.
 */

extern void actrapret(void);
extern void acsysret(void);

Mach*
getac(Proc *p, int core)
{
	int i;
	Mach *mp;

	mp = nil;
	if(core == 0)
		panic("can't getac for a %s", rolename[NIXTC]);
	lock(&nixaclock);
	if(waserror()){
		unlock(&nixaclock);
		nexterror();
	}
	if(core > 0){
		if(core >= MACHMAX)
			error("no such core");
		mp = sys->machptr[core];
		if(mp == nil || mp->online == 0 || mp->proc != nil)
			error("core not online or busy");
		if(mp->nixtype != NIXAC)
			error("core is not an AC");
	Found:
		mp->proc = p;
	}else{
		for(i = 0; i < MACHMAX; i++)
			if((mp = sys->machptr[i]) != nil && mp->online && mp->nixtype == NIXAC)
				if(mp->proc == nil)
					goto Found;
		error("not enough cores");
	}
	unlock(&nixaclock);
	poperror();
	return mp;
}

/*
 * BUG:
 * The AC must not accept interrupts while in the kernel,
 * or we must be prepared for nesting them, which we are not.
 * This is important for note handling, because postnote()
 * assumes that it's ok to send an IPI to an AC, no matter its
 * state. The /proc interface also assumes that.
 * 
 */
void
intrac(Proc *p)
{
	Mach *ac;

	ac = p->ac;
	if(ac == nil){
		DBG("intrac: Proc.ac is nil. no ipi sent.\n");
		return;
	}
	/*
	 * It's ok if the AC gets idle in the mean time.
	 */
	DBG("intrac: ipi to cpu%d\n", ac->machno);
	apicipi(ac->apicno);
}

void
putac(Mach *m)
{
	mfence();
	m->proc = nil;
}

void
stopac(void)
{
	Mach *mp;

	mp = up->ac;
	if(mp == nil)
		return;
	if(mp->proc != up)
		panic("stopac");

	lock(&nixaclock);
	up->ac = nil;
	mp->proc = nil;
	unlock(&nixaclock);

	/* TODO:
	 * send sipi to up->ac, it would rerun squidboy(), and
	 * wait for us to give it a function to run.
	 */
}

/*
 * Functions starting with ac... are run in the application core.
 * All other functions are run by the time-sharing cores.
 */

typedef void (*APfunc)(void);
extern int notify(Ureg*);

/*
 * run an arbitrary function with arbitrary args on an ap core
 * first argument is always pml4 for process
 * make a field and a struct for the args cache line.
 *
 * Returns the return-code for the ICC or -1 if the process was
 * interrupted while issuing the ICC.
 */
int
runac(Mach *mp, APfunc func, int flushtlb, void *a, int32_t n)
{
	uint8_t *dpg, *spg;

	if (n > sizeof(mp->icc->data))
		panic("runac: args too long");

	if(mp->online == 0)
		panic("Bad core");
	if(mp->proc != nil && mp->proc != up)
		panic("runapfunc: mach is busy with another proc?");

	memmove(mp->icc->data, a, n);
	if(flushtlb){
		DBG("runac flushtlb: cppml4 %#p %#p\n", mp->pml4->pa, m->pml4->pa);
		dpg = UINT2PTR(mp->pml4->va);
		spg = UINT2PTR(m->pml4->va);
		/* We should copy less:
		 *	memmove(dgp, spg, m->pml4->daddr * sizeof(PTE));
		 */
		memmove(dpg, spg, PTSZ);
		if(0){
			print("runac: upac pml4 %#p\n", up->ac->pml4->pa);
			dumpptepg(4, up->ac->pml4->pa);
		}
	}
	mp->icc->flushtlb = flushtlb;
	mp->icc->rc = ICCOK;

	DBG("runac: exotic proc on cpu%d\n", mp->machno);
	if(waserror()){
		qunlock(&up->debug);
		nexterror();
	}
	qlock(&up->debug);
	up->nicc++;
	up->state = Exotic;
	up->psstate = 0;
	qunlock(&up->debug);
	poperror();
	mfence();
	mp->icc->fn = func;
	sched();
	return mp->icc->rc;
}

/*
 * Cleanup done by runacore to pretend we are going back to user space.
 * We won't return and won't do what syscall() would normally do.
 * Do it here instead.
 */
static void
fakeretfromsyscall(Ureg *ureg)
{
	int s;

	poperror();	/* as syscall() would do if we would return */
	if(up->procctl == Proc_tracesyscall){	/* Would this work? */
		up->procctl = Proc_stopme;
		s = splhi();
		procctl(up);
		splx(s);
	}

	up->insyscall = 0;
	/* if we delayed sched because we held a lock, sched now */
	if(up->delaysched){
		sched();
		splhi();
	}
	kexit(ureg);
}

/*
 * Move the current process to an application core.
 * This is performed at the end of execac(), and
 * we pretend to be returning to user-space, but instead we
 * dispatch the process to another core.
 * 1. We do the final bookkeeping that syscall() would do after
 *    a return from sysexec(), because we are not returning.
 * 2. We dispatch the process to an AC using an ICC.
 *
 * This function won't return unless the process is reclaimed back
 * to the time-sharing core, and is the handler for the process
 * to deal with traps and system calls until the process dies.
 *
 * Remember that this function is the "line" between user and kernel
 * space, it's not expected to raise|handle any error.
 *
 * We install a safety error label, just in case we raise errors,
 * which we shouldn't. (noerrorsleft knows that for exotic processes
 * there is an error label pushed by us).
 */
void
runacore(void)
{
	Ureg *ureg;
	void (*fn)(void);
	int rc, flush, s;
	char *n;
	uint64_t t1;

	if(waserror())
		panic("runacore: error: %s\n", up->errstr);
	ureg = up->dbgreg;
	fakeretfromsyscall(ureg);
	fpusysrfork(ureg);

	procpriority(up, PriKproc, 1);
	rc = runac(up->ac, actouser, 1, nil, 0);
	procpriority(up, PriNormal, 0);
	for(;;){
		t1 = fastticks(nil);
		flush = 0;
		fn = nil;
		switch(rc){
		case ICCTRAP:
			s = splhi();
			m->cr2 = up->ac->cr2;
			DBG("runacore: trap %ulld cr2 %#ullx ureg %#p\n",
				ureg->type, m->cr2, ureg);
			switch(ureg->type){
			case IdtIPI:
				if(up->procctl || up->nnote)
					notify(up->dbgreg);
				if(up->ac == nil)
					goto ToTC;
				kexit(up->dbgreg);
				break;
			case IdtNM:
			case IdtMF:
			case IdtXF:
				/* these are handled in the AC;
				 * If we get here, they left in m->icc->data
				 * a note to be posted to the process.
				 * Post it, and make the vector a NOP.
				 */
				n = up->ac->icc->note;
				if(n != nil)
					postnote(up, 1, n, NDebug);
				ureg->type = IdtIPI;		/* NOP */
				break;
			default:
				cr3put(m->pml4->pa);
				if(0 && ureg->type == IdtPF){
					print("before PF:\n");
					print("AC:\n");
					dumpptepg(4, up->ac->pml4->pa);
					print("\n%s:\n", rolename[NIXTC]);
					dumpptepg(4, m->pml4->pa);
				}
				trap(ureg);
			}
			splx(s);
			flush = 1;
			fn = actrapret;
			break;
		case ICCSYSCALL:
			DBG("runacore: syscall ax %#ullx ureg %#p\n",
				ureg->ax, ureg);
			cr3put(m->pml4->pa);
			//syscall(ureg->ax, ureg);
			flush = 1;
			fn = acsysret;
			if(0)
			if(up->nqtrap > 2 || up->nsyscall > 1)
				goto ToTC;
			if(up->ac == nil)
				goto ToTC;
			break;
		default:
			panic("runacore: unexpected rc = %d", rc);
		}
		up->tctime += fastticks2us(fastticks(nil) - t1);
		procpriority(up, PriExtra, 1);
		rc = runac(up->ac, fn, flush, nil, 0);
		procpriority(up, PriNormal, 0);
	}
ToTC:
	/*
	 *  to procctl, then syscall,  to 
	 *  be back in the TC
	 */
	DBG("runacore: up %#p: return\n", up);
}

extern ACVctl *acvctl[];

void
actrapenable(int vno, char* (*f)(Ureg*, void*), void* a, char *name)
{
	ACVctl *v;

	if(vno < 0 || vno >= 256)
		panic("actrapenable: vno %d\n", vno);
	v = malloc(sizeof(Vctl));
	v->f = f;
	v->a = a;
	v->vno = vno;
	strncpy(v->name, name, KNAMELEN);
	v->name[KNAMELEN-1] = 0;

	if(acvctl[vno])
		panic("AC traps can't be shared");
	acvctl[vno] = v;
}


