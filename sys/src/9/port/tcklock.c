/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


/*
 * Ticket locks.
 * D. P. Reed and R. K. Kanodia. 
 * Synchronization with Eventcounts and Sequencers. 
 * Communications of the ACM, 22(2):115–23, Feb. 1979.
 *
 * A variant is used in Linux.
 *
 * These are here to measure them and compare wrt taslocks.
 * If there's no difference, these should go
 * (because taslocks are known to be robust and don't have bugs).
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/edf.h"

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
	Proc *p;

	p = l->p;
	print("lock %#p loop key %#ux pc %#p held by pc %#p proc %d\n",
		l, l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

static uint32_t
getuser(uint32_t key)
{
	return key & 0xFFFF;
}

static uint32_t
getticket(uint32_t key)
{
	return (key>>16) & 0xFFFF;
}

static uint32_t
incuser(uint32_t *key)
{
	uint32_t old, new;

	do{
		old = *key;
		new = (old&0xFFFF0000) | ((old+1)&0xFFFF);
	}while(!cas32(key, old, new));
	return getuser(new);
}

static uint32_t
incticket(uint32_t *key)
{
	uint32_t old, new;

	do{
		old = *key;
		
		new = ((old+0x10000)&0xFFFF0000) | (old&0xFFFF);
	}while(!cas32(key, old, new));
	return getticket(new);
}

static uint32_t
myticket(uint32_t user)
{
	return (user-1) & 0xFFFF;
}

int
lock(Lock *l)
{
	int i;
	uintptr_t pc;
	uint32_t user;
	uint64_t t0;

	pc = getcallerpc(&l);
	lockstats.locks++;
	if(up)
		ainc(&m->externup->nlocks);	/* prevent being scheded */
	cycles(&t0);
	user = incuser(&l->key);
	if(getticket(l->key) != myticket(user)){
		if(up)
			adec(&m->externup->nlocks);
		lockstats.glare++;
		i = 0;
		while(getticket(l->key) != myticket(user)){
			if(conf.nmach < 2 && up && m->externup->edf && (up->edf->flags & Admitted)){
				/*
				 * Priority inversion, yield on a uniprocessor; on a
				 * multiprocessor, the other processor will unlock
				 */
				print("inversion %#p pc %#p proc %d held by pc %#p proc %d\n",
					l, pc, up ? m->externup->pid : 0, l->pc, l->p ? l->p->pid : 0);
				m->externup->edf->d = todget(nil);	/* yield to process with lock */
			}
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(up)
			ainc(&m->externup->nlocks);
	}
	l->pc = pc;
	l->p = up;
	l->m = m;
	l->isilock = 0;
	if(up)
		m->externup->lastlock = l;
	if(l != &waitstatslk)
		addwaitstat(pc, t0, WSlock);
	return 0;
}

void
ilock(Lock *l)
{
	Mpl pl;
	uintptr_t pc;
	uint64_t t0;
	uint32_t user;

	pc = getcallerpc(&l);
	lockstats.locks++;

	pl = splhi();
	cycles(&t0);

	user = incuser(&l->key);
	if(getticket(l->key) != myticket(user)){
		splx(pl);
		while(getticket(l->key) != myticket(user))
			;
		pl = splhi();
	}
	m->ilockdepth++;
	if(up)
		m->externup->lastilock = l;
	l->pl = pl;
	l->pc = pc;
	l->p = up;
	l->isilock = 1;
	l->m = m;
	if(l != &waitstatslk)
		addwaitstat(pc, t0, WSlock);
}

int
canlock(Lock *l)
{
	Lock try, new;
	uintptr_t pc;
	uint64_t t0;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(up)
		ainc(&m->externup->nlocks);	/* prevent being scheded */
	cycles(&t0);

	try = *l;
	if(getuser(try.key) != getticket(try.key)){
	Cant:
		if(up)
			adec(&m->externup->nlocks);
		return 0;
	}
	new = try;
	incuser(&new.key);
	if(!cas32(&l->key, try.key, new.key))
		goto Cant;
	l->pc = pc;
	l->p = up;
	l->m = m;
	if(up)
		m->externup->lastlock = l;
	l->isilock = 0;
	return 1;
}

void
unlock(Lock *l)
{
	if(getticket(l->key) == getuser(l->key))
		print("unlock: not locked: pc %#p\n", getcallerpc(&l));
	if(l->isilock)
		print("unlock of ilock: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(l->p != up)
		print("unlock: up changed: pc %#p, acquired at pc %#p,"
			" lock p %#p, unlock up %#p\n", getcallerpc(&l), l->pc, l->p, up);
	l->m = nil;
	incticket(&l->key);

	if(up && adec(&m->externup->nlocks) == 0 && up->delaysched && islo()){
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
	Mpl pl;

	if(getticket(l->key) == getuser(l->key))
		print("iunlock: not locked: pc %#p\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(islo())
		print("iunlock while lo: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(l->m != m){
		print("iunlock by cpu%d, locked by cpu%d: pc %#p, held by %#p\n",
			m->machno, l->m->machno, getcallerpc(&l), l->pc);
	}

	pl = l->pl;
	l->m = nil;
	incticket(&l->key);
	m->ilockdepth--;
	if(up)
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
