#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

struct {
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
lockloop(Lock *l, ulong pc)
{
	Proc *p;

	p = l->p;
	print("lock loop key 0x%lux pc 0x%lux held by pc 0x%lux proc %lud\n",
		l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);

	if(up && up->state == Running && islo())
		sched();
}

void
lock(Lock *l)
{
	int i, cansched;
	ulong pc, oldpri;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(tas(&l->key) == 0){
		l->pc = pc;
		l->p = up;
		l->isilock = 0;
		return;
	}

	lockstats.glare++;
	cansched = up != nil && up->state == Running;
	if(cansched){
		oldpri = up->priority;
		up->lockwait = 1;
		up->priority = PriLock;
	} else
		oldpri = 0;

	for(;;){
		lockstats.inglare++;
		i = 0;
		while(l->key){
			if(conf.nmach < 2 && cansched){
				if(i++ > 1000){
					i = 0;
					lockloop(l, pc);
				}
				sched();
			} else {
				if(i++ > 100000000){
					i = 0;
					lockloop(l, pc);
				}
			}
		}
		if(tas(&l->key) == 0){
			l->pc = pc;
			l->p = up;
			l->isilock = 0;
			if(cansched){
				up->lockwait = 0;
				up->priority = oldpri;
			}
			return;
		}
	}
}

void
ilock(Lock *l)
{
	ulong x;
	ulong pc, oldpri;
	int cansched;

	pc = getcallerpc(&l);
	lockstats.locks++;

	x = splhi();
	if(tas(&l->key) == 0){
		l->sr = x;
		l->pc = pc;
		l->p = up;
		l->isilock = 1;
		return;
	}

	lockstats.glare++;
	cansched = up != nil && up->state == Running;
	if(cansched){
		oldpri = up->priority;
		up->lockwait = 1;
		up->priority = PriLock;
	} else
		oldpri = 0;
	if(conf.nmach < 2)
{
dumplockmem("ilock:", l);
		panic("ilock: no way out: pc %uX\n", pc);
}

	for(;;){
		lockstats.inglare++;
		splx(x);
		while(l->key)
			;
		x = splhi();
		if(tas(&l->key) == 0){
			l->sr = x;
			l->pc = pc;
			l->p = up;
			l->isilock = 1;
			if(cansched){
				up->lockwait = 0;
				up->priority = oldpri;
			}
			return;
		}
	}
}

int
canlock(Lock *l)
{
	if(tas(&l->key))
		return 0;

	l->pc = getcallerpc(&l);
	l->p = up;
	l->isilock = 0;
	return 1;
}

void
unlock(Lock *l)
{

	if(l->key == 0)
		print("unlock: not locked: pc %luX\n", getcallerpc(&l));
	if(l->isilock)
		print("unlock of ilock: pc %lux, held by %lux\n", getcallerpc(&l), l->pc);
	l->pc = 0;
	l->key = 0;
	coherence();
}

void
iunlock(Lock *l)
{
	ulong sr;

	if(l->key == 0)
		print("iunlock: not locked: pc %luX\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %lux, held by %lux\n", getcallerpc(&l), l->pc);

	sr = l->sr;
	l->pc = 0;
	l->key = 0;
	coherence();

	m->splpc = getcallerpc(&l);
	splxpc(sr);
}
