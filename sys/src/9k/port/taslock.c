/*
 * (i)(un)lock, canlock - spinlocks with test-and-set, for short-lived locks
 *
 * recently added more coherence calls to push changes to other cpus faster
 * and ensure current data from other cpus when reading.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/edf.h"

enum {
	/*
	 * 10^8 loops at ~4 GHz should take ~25 ms.  machines now
	 * may have a lot of memory to walk, so give a little longer.
	 */
	Maxattempts = 1ull<<31,	/* after this many cycles, declare loop */
};

uvlong maxlockcycles;
uvlong maxilockcycles;
ulong maxlockpc;
ulong maxilockpc;

struct
{
	ulong	locks;
	ulong	glare;
	ulong	inglare;
} lockstats;

/* provide better diagnostics than adec alone */
static int
deccnt(int *cntp, char *who, uintptr caller)
{
	int x, old;
	Mpl pl;

	pl = splhi();
	SET(old); USED(old);
#ifdef PARANOID
	coherence();
	old = *cntp;
	if (old <= 0)
		panic("deccnt: ref %d <= 0 at entry; %s callerpc=%#p",
			old, who, caller);
#endif
	x = adec(cntp);
	if (x < 0)
		panic("deccnt: ref %d < 0 after adec; %s callerpc=%#p",
			x, who, caller);
	splx(pl);
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

static void
lockloop(Lock *l, uintptr pc)
{
	Proc *p;

	p = l->p;
	iprint("lock %#p loop key %#ux pc %#p held by pc %#p proc %d\n",
		l, l->key, pc, l->pc, p? p->pid: 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

static uvlong
chkstuck(Lock *l, uintptr pc, uvlong stcyc)
{
	uvlong nowcyc;

	cycles(&nowcyc);
	if(nowcyc - stcyc > Maxattempts){	/* might we be stuck? */
		lockloop(l, pc);
		cycles(&stcyc);
	}
	return stcyc;
}

static void
userlock(Lock *lck, char *func, uintptr pc)
{
	panic("%s: nil or user-space Lock* %#p; called from %#p", func, lck, pc);
}

/* call once we have l->key (via TAS) */
static void
takelock(Lock *l, uintptr pc)
{
	if(up)
		up->lastlock = l;
	l->pc = pc;
	l->p = up;
	l->isilock = 0;
#ifdef LOCKCYCLES
	cycles(&l->lockcycles);
#endif
	coherence();		/* make changes visible to other cpus */
}

/*
 * Priority inversion, yield on a uniprocessor;
 * on a multiprocessor, another processor will unlock.
 * Not a common code path.
 */
static void
edfpriinv(Lock *l, uintptr pc)
{
	print("inversion %#p pc %#p proc %d held by pc %#p proc %d\n",
		l, pc, up? up->pid: 0, l->pc, l->p? l->p->pid: 0);
	up->edf->d = todget(nil);	/* yield to process with lock */
}

void
portmwaitforlock(Lock *, ulong)
{
	pause();
}

/* only if portmwait can be interrupted will we be able to detect lock loops */
void
realmwaitforlock(Lock *l, ulong keyval)
{
	if (l->key == keyval) {
		portmwait();
		portmonitor(&l->key);
	}
}

static void
waitforlock(Lock *l, uintptr pc, Edf *edf)
{
	ulong keyval;
	uvlong stcyc;

	cycles(&stcyc);
	portmonitor(&l->key);
	while ((keyval = l->key) != 0){
		/* lock's busy; wait a bit for it to be released */
		if(edf && edf->flags & Admitted)
			edfpriinv(l, pc);
		stcyc = chkstuck(l, pc, stcyc);
		mwaitforlock(l, keyval);
	}
}

int
lock(Lock *l)
{
	uintptr pc;
	Edf *edf;

	pc = getcallerpc(&l);
	/*
	 * we can be called from low addresses before the mmu is on.
	 */
	if (l == nil)
		userlock(l, "lock", pc);
	coherence();	/* ensure changes from other cpus are visible */
	if (l->noninit)
		panic("lock: lock %#p is not a lock, from %#p", l, pc);
	lockstats.locks++;
	if(up)
		ainc(&up->nlocks);	/* prevent being scheded */
	if(TAS(&l->key) == 0){
		takelock(l, pc);
		return 0;		/* got it on first try */
	}
	if(up) {
		deccnt(&up->nlocks, "lock", pc);
		if(l->p == up)
			panic("lock: deadlock acquiring lock held by same proc,"
				" from %#p", pc);
	}
	lockstats.glare++;
	edf = sys->nonline <= 1 && up? up->edf: nil;
	for(;;){
		lockstats.inglare++;
		coherence();	/* ensure changes from other cpus are visible */
		waitforlock(l, pc, edf);

		/* try again to grab the lock */
		if(up)
			ainc(&up->nlocks);
		if(TAS(&l->key) == 0){
			takelock(l, pc);
			return 1;	/* got it, but not on first try */
		}
		if(up)
			deccnt(&up->nlocks, "lock", pc);
	}
}

/* waits for the lock at entry PL, returns at high PL */
void
ilock(Lock *l)
{
	ulong keyval;
	uvlong stcyc;
	Mreg s;
	uintptr pc;

	pc = getcallerpc(&l);
	if (l == nil)
		userlock(l, "ilock", pc);
	if (l->noninit) 
		panic("ilock: lock %#p is not a lock (noninit = %#lux), from %#p",
			l, l->noninit, pc);
	lockstats.locks++;

	s = splhi();
	if(TAS(&l->key) != 0){
		lockstats.glare++;
		/*
		 * Cannot also check l->pc, l->m, or l->isilock here
		 * because they might just not be set yet, or
		 * (for pc and m) the lock might have just been unlocked.
		 */
		cycles(&stcyc);
		do {
			lockstats.inglare++;
			/*
			 * we may be waiting for an iunlock in or triggered
			 * from an interrupt service routine and could be
			 * a uniprocessor.
			 */
			splx(s);
			portmonitor(&l->key);
			while((keyval = l->key) != 0) {
				stcyc = chkstuck(l, pc, stcyc);
				mwaitforlock(l, keyval);
			}
			splhi();
		} while (TAS(&l->key) != 0);	/* try to grab lock again */
	}
	m->ilockdepth++;
	m->ilockpc = pc;
	if(up)
		up->lastilock = l;
	l->sr = s;
	l->pc = pc;
	l->p = up;
	l->isilock = 1;
	l->m = m;
#ifdef LOCKCYCLES
	cycles(&l->lockcycles);
#endif
	coherence();		/* make changes visible to other cpus */
}

int
canlock(Lock *l)
{
	uintptr pc;

	pc = getcallerpc(&l);
	if (l == nil)
		userlock(l, "canlock", pc);
	coherence();
	if(up)
		ainc(&up->nlocks);
	if(TAS(&l->key) != 0){		/* failed to acquire the lock? */
		if(up)
			deccnt(&up->nlocks, "canlock", pc);
		return 0;
	}

	takelock(l, pc);
	l->m = m;
	return 1;
}

/*
 * Call sched if the need arose while locks were held, but
 * don't do it from interrupt routines, hence the islo() test.
 */
static void
delayedsched(void)
{
	if (up->delaysched && islo())
		sched();
}

void
unlock(Lock *l)
{
	int decok;
	uintptr pc;
#ifdef LOCKCYCLES
	uvlong x;

	cycles(&x);
	l->lockcycles = x - l->lockcycles;
	if(l->lockcycles > maxlockcycles){
		maxlockcycles = l->lockcycles;
		maxlockpc = l->pc;
	}
#endif
	pc = getcallerpc(&l);
	if (l == nil)
		userlock(l, "unlock", pc);
	coherence();	/* ensure changes from other cpus are visible */
	if(l->key == 0)
		iprint("unlock: not locked: pc %#p\n", pc);
	if(l->isilock)
		iprint("unlock of ilock: pc %#p, held by %#p\n", pc, l->pc);

	/*
	 * only the Lock-holding process should release it.  otherwise,
	 * the wrong up->nlocks will be decremented.  a Lock may have been
	 * acquired when up was nil and is being released after setting it,
	 * but Locks are not supposed to be held for long (in particular,
	 * across sleeps or sched calls).
	 *
	 * if l->p is nil, up->nlocks should not have been incremented when
	 * locking, thus should not be decremented here.
	 * l->p == up should always be true.
	 */
	decok = up != nil && l->p == up;
	if (l->p != up)
		iprint("unlock: l->p changed: pc %#p, acquired at pc %#p, "
			"lock p %#p != unlock up %#p\n", pc, l->pc, l->p, up);
	l->m = nil;
	l->p = nil;
	coherence();		/* make changes visible to other cpus */
	l->key = 0;		/* this releases the lock */
	coherence();		/* make changes visible to other cpus */
	/*
	 * Call sched if the need arose while locks were held.
	 */
	if (up)
		if (decok) {
			if (deccnt(&up->nlocks, "unlock", pc) == 0)
				delayedsched();
		} else
			if (up->nlocks == 0)
				delayedsched();
	/* contenders for this lock will be spinning, no need to wake them */
}

void
iunlock(Lock *l)
{
	Mreg s;
#ifdef LOCKCYCLES
	uvlong x;

	cycles(&x);
	l->lockcycles = x - l->lockcycles;
	if(l->lockcycles > maxilockcycles){
		maxilockcycles = l->lockcycles;
		maxilockpc = l->pc;
	}
#endif
	if (l == nil)
		userlock(l, "iunlock", getcallerpc(&l));
	coherence();	/* ensure changes from other cpus are visible */
	if(l->key == 0)
		print("iunlock: not locked: pc %#p\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %#p, held by %#p\n",
			getcallerpc(&l), l->pc);
	if(islo())
		print("iunlock while lo: pc %#p, held by %#p\n",
			getcallerpc(&l), l->pc);
	if(l->m != m)
		print("iunlock by cpu%d, locked by cpu%d: pc %#p, held by %#p\n",
			m->machno, l->m->machno, getcallerpc(&l), l->pc);

	s = l->sr;
	l->m = nil;
	l->p = nil;
	coherence();		/* make changes visible to other cpus */
	l->key = 0;		/* this releases the lock */
	coherence();		/* make changes visible to other cpus */
	m->ilockdepth--;
	if(up)
		up->lastilock = nil;
	/* contenders for this lock will be spinning, no need to wake them */
	splx(s);
}
