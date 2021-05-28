#include "include.h"

#undef LOCKCYCLES

#ifdef LOCKCYCLES
uvlong maxlockcycles;
uvlong maxilockcycles;
ulong maxlockpc;
ulong maxilockpc;
#endif

struct
{
	ulong	locks;
	ulong	glare;
	ulong	inglare;
} lockstats;

static void
inccnt(Ref *r)
{
	_xinc(&r->ref);
}

static int
deccnt(Ref *r)
{
	int x;

	x = _xdec(&r->ref);
	assert(x >= 0);
//	if(x < 0)
//		panic("deccnt pc=%#p", getcallerpc(&r));
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
lockloop(Lock *l, uintptr pc)
{
	print("lock %#p loop key %#ux pc %#p\n", l, l->key, pc);
}

int
lock(Lock *l)
{
	int i;
	uintptr pc;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(TAS(&l->key) == 0){
		l->pc = pc;
		l->isilock = 0;
#ifdef LOCKCYCLES
		cycles(&l->lockcycles);
#endif
		return 0;
	}

	lockstats.glare++;
	for(;;){
		lockstats.inglare++;
		i = 0;
		while(l->key){
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(TAS(&l->key) == 0){
			l->pc = pc;
			l->isilock = 0;
#ifdef LOCKCYCLES
			cycles(&l->lockcycles);
#endif
			return 1;
		}
	}
}

void
ilock(Lock *l)
{
	ulong sr;
	uintptr pc;

	pc = getcallerpc(&l);
	lockstats.locks++;

	sr = splhi();
	if(TAS(&l->key) != 0){
		lockstats.glare++;
		/*
		 * Cannot also check l->pc and l->m here because
		 * they might just not be set yet, or the lock might 
		 * even have been let go.
		 */
		if(!l->isilock){
			dumplockmem("ilock:", l);
			print("corrupt ilock %#p pc=%#p m=%#p isilock=%d", 
				l, l->pc, l->m, l->isilock);
			assert(0);
		}
		if(l->m == MACHP(m->machno)) {
			print("ilock: deadlock on cpu%d pc=%#p lockpc=%#p\n", 
				m->machno, pc, l->pc);
			assert(0);
		}
		for(;;){
			lockstats.inglare++;
			splx(sr);
			while(l->key)
				;
			sr = splhi();
			if(TAS(&l->key) == 0)
				goto acquire;
		}
	}
acquire:
//	m->ilockdepth++;
	l->sr = sr;
	l->pc = pc;
	l->isilock = 1;
	l->m = MACHP(m->machno);
#ifdef LOCKCYCLES
	cycles(&l->lockcycles);
#endif
}

int
canlock(Lock *l)
{
	if(TAS(&l->key))
		return 0;

	l->pc = getcallerpc(&l);
	l->m = MACHP(m->machno);
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
		print("unlock of ilock: pc %#p, held by %#p\n",
			getcallerpc(&l), l->pc);
	l->m = nil;
	l->key = 0;
	coherence();
}

void
iunlock(Lock *l)
{
	ulong sr;

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

	sr = l->sr;
	l->m = nil;
	l->key = 0;
	coherence();
//	m->ilockdepth--;
	splx(sr);
}
