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

#include "../port/edf.h"

/*
 * measure max lock cycles and max lock waiting time.
 */
#define	LOCKCYCLES	0

uint64_t maxlockcycles;
uint64_t maxilockcycles;
uintptr_t maxlockpc;
uintptr_t maxilockpc;

Lockstats lockstats;
Waitstats waitstats;
Lock waitstatslk;

static void
newwaitstats(void)
{
	if(waitstats.pcs != nil)
		return;
	waitstats.pcs = malloc(NWstats * sizeof waitstats.pcs[0]);
	waitstats.ns = malloc(NWstats * sizeof waitstats.ns[0]);
	waitstats.wait = malloc(NWstats * sizeof waitstats.wait[0]);
	waitstats.total = malloc(NWstats * sizeof waitstats.total[0]);
	waitstats.type = malloc(NWstats * sizeof waitstats.type[0]);
}

void
startwaitstats(int on)
{
	newwaitstats();
	mfence();
	waitstats.on = on;
	print("lockstats %s\n", on?"on":"off");
}

void
clearwaitstats(void)
{
	newwaitstats();
	memset(waitstats.ns, 0, NWstats * sizeof(int));
	memset(waitstats.wait, 0, NWstats * sizeof(uint64_t));
	memset(waitstats.total, 0, NWstats * sizeof(uint64_t));
}

void
addwaitstat(uintptr_t pc, uint64_t t0, int type)
{
	uint i;
	uint64_t w;

	if(waitstats.on == 0)
		return;

	cycles(&w);
	w -= t0;
	mfence();
	for(i = 0; i < NWstats; i++)
		if(waitstats.pcs[i] == pc){
			ainc(&waitstats.ns[i]);
			if(w > waitstats.wait[i])
				waitstats.wait[i] = w;	/* race but ok */
			waitstats.total[i] += w;		/* race but ok */
			return;
		}
	if(!canlock(&waitstatslk))
		return;

	for(i = 0; i < NWstats; i++)
		if(waitstats.pcs[i] == pc){
			ainc(&waitstats.ns[i]);
			if(w > waitstats.wait[i])
				waitstats.wait[i] = w;	/* race but ok */
			waitstats.total[i] += w;
			unlock(&waitstatslk);
			return;
		}

	for(i = 0; i < NWstats; i++)
		if(waitstats.pcs[i] == 0){
			waitstats.ns[i] = 1;
			waitstats.type[i] = type;
			waitstats.wait[i] = w;
			waitstats.total[i] = w;
			mfence();
			waitstats.pcs[i] = pc;
			waitstats.npcs++;
			break;
		}

	unlock(&waitstatslk);
}

void
lockloop(Lock *l, uintptr_t pc)
{
	Mach *m = machp();
	Proc *p;

	p = l->p;
	print("lock %#p loop key %#ux pc %#p held by pc %#p proc %d\n",
		l, l->key, pc, l->_pc, p ? p->pid : 0);
	dumpaproc(m->externup);
	if(p != nil)
		dumpaproc(p);
}

int
lock(Lock *l)
{
	Mach *m = machp();
	int i;
	uintptr_t pc;
	uint64_t t0;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(m->externup)
		ainc(&m->externup->nlocks);	/* prevent being scheded */
	if(TAS(&l->key) == 0){
		if(m->externup)
			m->externup->lastlock = l;
		l->_pc = pc;
		l->p = m->externup;
		l->isilock = 0;
		if(LOCKCYCLES)
			cycles(&l->lockcycles);

		return 0;
	}
	if(m->externup)
		adec(&m->externup->nlocks);

	cycles(&t0);
	lockstats.glare++;
	for(;;){
		lockstats.inglare++;
		i = 0;
		while(l->key){
			if(sys->nmach < 2 && m->externup && m->externup->edf && (m->externup->edf->flags & Admitted)){
				/*
				 * Priority inversion, yield on a uniprocessor; on a
				 * multiprocessor, the other processor will unlock
				 */
				print("inversion %#p pc %#p proc %d held by pc %#p proc %d\n",
					l, pc, m->externup ? m->externup->pid : 0, l->_pc, l->p ? l->p->pid : 0);
				m->externup->edf->d = todget(nil);	/* yield to process with lock */
			}
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(m->externup)
			ainc(&m->externup->nlocks);
		if(TAS(&l->key) == 0){
			if(m->externup)
				m->externup->lastlock = l;
			l->_pc = pc;
			l->p = m->externup;
			l->isilock = 0;
			if(LOCKCYCLES)
				cycles(&l->lockcycles);
			if(l != &waitstatslk)
				addwaitstat(pc, t0, WSlock);
			return 1;
		}
		if(m->externup)
			adec(&m->externup->nlocks);
	}
}

void
ilock(Lock *l)
{
	Mach *m = machp();
	Mpl pl;
	uintptr_t pc;
	uint64_t t0;

	pc = getcallerpc(&l);
	lockstats.locks++;

	pl = splhi();
	if(TAS(&l->key) != 0){
		cycles(&t0);
		lockstats.glare++;
		/*
		 * Cannot also check l->pc, l->m, or l->isilock here
		 * because they might just not be set yet, or
		 * (for pc and m) the lock might have just been unlocked.
		 */
		for(;;){
			lockstats.inglare++;
			splx(pl);
			while(l->key)
				;
			pl = splhi();
			if(TAS(&l->key) == 0){
				if(l != &waitstatslk)
					addwaitstat(pc, t0, WSlock);
				goto acquire;
			}
		}
	}
acquire:
	machp()->ilockdepth++;
	if(m->externup)
		m->externup->lastilock = l;
	l->pl = pl;
	l->_pc = pc;
	l->p = m->externup;
	l->isilock = 1;
	l->m = machp();
	if(LOCKCYCLES)
		cycles(&l->lockcycles);
}

int
canlock(Lock *l)
{
	Mach *m = machp();
	if(m->externup)
		ainc(&m->externup->nlocks);
	if(TAS(&l->key)){
		if(m->externup)
			adec(&m->externup->nlocks);
		return 0;
	}

	if(m->externup)
		m->externup->lastlock = l;
	l->_pc = getcallerpc(&l);
	l->p = m->externup;
	l->m = machp();
	l->isilock = 0;
	if(LOCKCYCLES)
		cycles(&l->lockcycles);

	return 1;
}

void
unlock(Lock *l)
{
	Mach *m = machp();
	uint64_t x;

	if(LOCKCYCLES){
		cycles(&x);
		l->lockcycles = x - l->lockcycles;
		if(l->lockcycles > maxlockcycles){
			maxlockcycles = l->lockcycles;
			maxlockpc = l->_pc;
		}
	}

	if(l->key == 0)
		print("unlock: not locked: pc %#p\n", getcallerpc(&l));
	if(l->isilock)
		print("unlock of ilock: pc %#p, held by %#p\n", getcallerpc(&l), l->_pc);
	if(l->p != m->externup)
		print("unlock: up changed: pc %#p, acquired at pc %#p, lock p %#p, unlock up %#p\n", getcallerpc(&l), l->_pc, l->p, m->externup);
	l->m = nil;
	l->key = 0;
	coherence();

	if(m->externup && adec(&m->externup->nlocks) == 0 && m->externup->delaysched && islo()){
		/*
		 * Call sched if the need arose while locks were held
		 * But, don't do it from interrupt routines, hence the islo() test
		 */
		sched();
	}
}

void
iunlock(Lock *l)
{
	Mach *m = machp();
	Mpl pl;
	uint64_t x;

	if(LOCKCYCLES){
		cycles(&x);
		l->lockcycles = x - l->lockcycles;
		if(l->lockcycles > maxilockcycles){
			maxilockcycles = l->lockcycles;
			maxilockpc = l->_pc;
		}
	}

	if(l->key == 0)
		print("iunlock: not locked: pc %#p\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %#p, held by %#p\n", getcallerpc(&l), l->_pc);
	if(islo())
		print("iunlock while lo: pc %#p, held by %#p\n", getcallerpc(&l), l->_pc);
	if(l->m != machp()){
		print("iunlock by cpu%d, locked by cpu%d: pc %#p, held by %#p\n",
			machp()->machno, l->m->machno, getcallerpc(&l), l->_pc);
	}

	pl = l->pl;
	l->m = nil;
	l->key = 0;
	coherence();
	machp()->ilockdepth--;
	if(m->externup)
		m->externup->lastilock = nil;
	splx(pl);
}

void
portmwait(void *value)
{
	while (*(void**)value == nil)
		;
}

void (*mwait)(void *) = portmwait;
