#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/edf.h"

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
lockloop(Lock *l, uintptr pc)
{
	Proc *p;

	p = l->p;
	print("lock %#p loop key %#ux pc %#p held by pc %#p proc %d\n",
		l, l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

int
lock(Lock *l)
{
	int i;
	uintptr pc;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(up)
		ainc(&up->nlocks);	/* prevent being scheded */
	if(TAS(&l->key) == 0){
		if(up)
			up->lastlock = l;
		l->pc = pc;
		l->p = up;
		l->isilock = 0;
		l->m = m;
#ifdef LOCKCYCLES
		cycles(&l->lockcycles);
#endif
		return 0;
	}
	if(up)
		adec(&up->nlocks);

	lockstats.glare++;
	for(;;){
		lockstats.inglare++;
		i = 0;
		while(l->key){
			if(sys->nonline < 2 && up && up->edf && (up->edf->flags & Admitted)){
				/*
				 * Priority inversion, yield on a uniprocessor; on a
				 * multiprocessor, the other processor will unlock
				 */
				print("inversion %#p pc %#p proc %d held by pc %#p proc %d\n",
					l, pc, up ? up->pid : 0, l->pc, l->p ? l->p->pid : 0);
				up->edf->d = todget(nil);	/* yield to process with lock */
			}
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(up)
			ainc(&up->nlocks);
		if(TAS(&l->key) == 0){
			if(up)
				up->lastlock = l;
			l->pc = pc;
			l->p = up;
			l->isilock = 0;
			l->m = m;
#ifdef LOCKCYCLES
			cycles(&l->lockcycles);
#endif
			return 1;
		}
		if(up)
			adec(&up->nlocks);
	}
}

void
ilock(Lock *l)
{
	Mreg s;
	uintptr pc;

	pc = getcallerpc(&l);
	lockstats.locks++;

	s = splhi();
	if(TAS(&l->key) != 0){
		lockstats.glare++;
		/*
		 * Cannot also check l->pc, l->m, or l->isilock here
		 * because they might just not be set yet, or
		 * (for pc and m) the lock might have just been unlocked.
		 */
		for(;;){
			lockstats.inglare++;
			splx(s);
			while(l->key)
				;
			s = splhi();
			if(TAS(&l->key) == 0)
				goto acquire;
		}
	}
acquire:
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
}

int
canlock(Lock *l)
{
	if(up)
		ainc(&up->nlocks);
	if(TAS(&l->key)){
		if(up)
			adec(&up->nlocks);
		return 0;
	}

	if(up)
		up->lastlock = l;
	l->pc = getcallerpc(&l);
	l->p = up;
	l->m = m;
	l->isilock = 0;
#ifdef LOCKCYCLES
	cycles(&l->lockcycles);
#endif
	return 1;
}

void
unlock(Lock *l)
{
#ifdef LOCKCYCLES
	uvlong x;
	cycles(&x);
	l->lockcycles = x - l->lockcycles;
	if(l->lockcycles > maxlockcycles){
		maxlockcycles = l->lockcycles;
		maxlockpc = l->pc;
	}
#endif

	if(l->key == 0)
		print("unlock: not locked: pc %#p\n", getcallerpc(&l));
	if(l->isilock)
		print("unlock of ilock: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(l->p != up)
		print("unlock: up changed: pc %#p, acquired at pc %#p, lock p %#p, unlock up %#p\n", getcallerpc(&l), l->pc, l->p, up);
	l->m = nil;
	l->key = 0;
	coherence();

	if(up && adec(&up->nlocks) == 0 && up->delaysched && islo()){
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

	if(l->key == 0)
		print("iunlock: not locked: pc %#p\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(islo())
		print("iunlock while lo: pc %#p, held by %#p\n", getcallerpc(&l), l->pc);
	if(l->m != m){
		print("iunlock by cpu%d, locked by cpu%d: pc %#p, held by %#p\n",
			m->machno, l->m->machno, getcallerpc(&l), l->pc);
	}

	s = l->sr;
	l->m = nil;
	l->key = 0;
	coherence();
	m->ilockdepth--;
	if(up)
		up->lastilock = nil;
	splx(s);
}
