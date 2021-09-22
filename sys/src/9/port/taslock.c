#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/edf.h"

enum {
	Notilock,
	Isilock,
};

long maxlockcycles;
long maxilockcycles;
long cumlockcycles;
long cumilockcycles;
ulong maxlockpc;
ulong maxilockpc;

/* only approximate counts on MPs because incremented by ++ not ainc() */
struct
{
	ulong	locks;
	ulong	glare;
	ulong	inglare;
} lockstats;

#define inccnt(r) ainc(&(r)->ref)

static int
deccnt(Ref *r, char *who, uintptr caller)
{
	int x, old;

	x = old = r->ref;
	/* imperfect attempt to avoid decrementing a zero ref count */
	if (old <= 0 || (x = adec(&r->ref)) < 0)
		panic("deccnt: r->ref %d <= 0; %s callerpc=%#p",
			old, who, caller);
	return x;
}

static void
dumplockmem(char *tag, Lock *l)
{
	uchar *cp;
	int i;

	iprint("%s: ", tag);
	cp = (uchar*)l;
	for(i = 0; i < 64; i++)
		iprint("%2.2ux ", cp[i]);
	iprint("\n");
}

void
lockloop(Lock *l, ulong pc)
{
	Proc *p;

	p = l->p;
	print("lock %#p loop key %#lux pc %#lux held by pc %#lux proc %lud\n",
		l, l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

/*
 * Priority inversion, yield on a uniprocessor;
 * on a multiprocessor, another processor will unlock.
 */
static void
edfunipriinversion(Lock *l, ulong pc)
{
	print("inversion %#p pc %#lux proc %lud held by pc %#lux proc %lud\n",
		l, pc, up? up->pid: 0, l->pc, l->p? l->p->pid: 0);
	/* yield to process with lock */
	up->edf->d = todget(nil);
}

static int
grablock(Lock *l, char *who, uintptr caller)
{
	if (up) {
		/* prevent being scheded until nlocks==0*/
		inccnt(&up->nlocks);
		if(tas(&l->key) == 0)
			return 0;
		deccnt(&up->nlocks, who, caller);
	} else
		if(tas(&l->key) == 0)
			return 0;
	return -1;
}

/* call once the lock is ours */
static void
ownlock(Lock *l, ulong pc, int isilock)
{
	if (up)
		up->lastlock = l;
	l->pc = pc;
	l->p = up;
	l->m = MACHP(m->machno);
	l->isilock = isilock;
#ifdef LOCKCYCLES
	l->lockcycles = -lcycles();
#endif
}

static void
waitforlock(Lock *l, ulong pc, Edf *edf)
{
	uvlong stcyc, nowcyc;

	cycles(&stcyc);
	while (l->key){
		/* lock's busy; wait a bit for it to be released */
		if(edf && edf->flags & Admitted)
			edfunipriinversion(l, pc);
		cycles(&nowcyc);
		if(nowcyc - stcyc > 100000000){	/* are we stuck? */
			lockloop(l, pc);
			cycles(&stcyc);
		}
	}
}

int
lock(Lock *l)
{
	int ret;
	ulong pc;
	Edf *edf;

	lockstats.locks++;
	pc = getcallerpc(&l);
	ret = 0;
	edf = conf.nmach <= 1 && up? up->edf: nil;
	while (grablock(l, "lock", pc) < 0) {
		/* didn't grab the lock? wait & try again */
		if (ret == 0) {
			lockstats.glare++;
			ret = 1;		/* took more than one try */
		} else
			lockstats.inglare++;
		waitforlock(l, pc, edf);
	}
	ownlock(l, pc, Notilock);
	return ret;				/* nobody looks at this value */
}

void
ilock(Lock *l)
{
	ulong x;

	lockstats.locks++;
	x = splhi();
	if(tas(&l->key) != 0){
		lockstats.glare++;
		/*
		 * Have to wait for l to be released.
		 * Cannot also check l->pc, l->m, or l->isilock here
		 * because they might just not be set yet, or
		 * (for pc and m) the lock might have just been unlocked.
		 */
		do{
			lockstats.inglare++;
			splx(x);
			while(l->key)	/* this is usually a short wait */
				;
			x = splhi();
			/* looks free, grab it quick */
		} while (tas(&l->key) != 0);
	}
	m->ilockdepth++;
	ownlock(l, getcallerpc(&l), Isilock);
	l->sr = x;
}

/* try to acquire lock once; return success boolean */
int
canlock(Lock *l)
{
	uintptr caller;

	caller = getcallerpc(&l);
	if (grablock(l, "canlock", caller) < 0)
		return 0;
	ownlock(l, caller, Notilock);
	return 1;
}

static void
givelockback(Lock *l)
{
	l->m = nil;
	l->key = 0;
	coherence();
}

void
unlock(Lock *l)
{
	uintptr caller;

#ifdef LOCKCYCLES
	l->lockcycles += lcycles();
	cumlockcycles += l->lockcycles;
	if(l->lockcycles > maxlockcycles){
		maxlockcycles = l->lockcycles;
		maxlockpc = l->pc;
	}
#endif
	caller = getcallerpc(&l);
	if(l->key == 0)
		print("unlock: not locked: pc %#p\n", caller);
	if(l->isilock)
		print("unlock of ilock: pc %#lux, held by %#lux\n", caller, l->pc);
	if(l->p != up)
		print("unlock: up changed: pc %#p, acquired at pc %#lux, "
			"lock p %#p, unlock up %#p\n", caller, l->pc, l->p, up);
	givelockback(l);
	if(up && deccnt(&up->nlocks, "unlock", caller) == 0 &&
	    up->delaysched && islo())
		/*
		 * Call sched if the need arose while locks were held.
		 * But don't do it from intr routines, hence the islo() test.
		 */
		sched();
}

ulong ilockpcs[0x100] = { [0xff] = 1 };
static int ilockpc;

void
iunlock(Lock *l)
{
	ulong sr;
	uintptr caller;

#ifdef LOCKCYCLES
	l->lockcycles += lcycles();
	cumilockcycles += l->lockcycles;
	if(l->lockcycles > maxilockcycles){
		maxilockcycles = l->lockcycles;
		maxilockpc = l->pc;
	}
	if(l->lockcycles > 2400)
		ilockpcs[ilockpc++ & 0xff] = l->pc;
#endif
	caller = getcallerpc(&l);
	if(l->key == 0)
		print("iunlock: not locked: pc %#p\n", caller);
	if(!l->isilock)
		print("iunlock of lock: pc %#p, held by %#lux\n", caller, l->pc);
	if(islo())
		print("iunlock while lo: pc %#p, held by %#lux\n", caller, l->pc);

	sr = l->sr;
	givelockback(l);
	m->ilockdepth--;
	if(up)
		up->lastilock = nil;
	splx(sr);
}

static void
profiling_after_iunlock(void)
{
	profiling_after_iunlock();
	profiling_after_iunlock();
}
