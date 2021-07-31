#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "edf.h"

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
	if(x < 0)
		panic("decref pc=0x%lux", getcallerpc(&r));
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
	print("lock 0x%lux loop key 0x%lux pc 0x%lux held by pc 0x%lux proc %lud\n",
		l, l->key, pc, l->pc, p ? p->pid : 0);
	dumpaproc(up);
	if(p != nil)
		dumpaproc(p);
}

int
lock(Lock *l)
{
	int i;
	ulong pc;

	pc = getcallerpc(&l);

	lockstats.locks++;
	if(up)
		inccnt(&up->nlocks);	/* prevent being scheded */
	if(tas(&l->key) == 0){
		if(up)
			up->lastlock = l;
		l->pc = pc;
		l->p = up;
		l->isilock = 0;
		return 0;
	}
	if(up)
		deccnt(&up->nlocks);

	lockstats.glare++;
	for(;;){
		lockstats.inglare++;
		i = 0;
		while(l->key){
			if(up && up->edf && (up->edf->flags & Admitted)){
				/* priority inversion, yield */
				print("inversion 0x%lux pc 0x%lux proc %lud held by pc 0x%lux proc %lud\n",
					l, pc, up ? up->pid : 0, l->pc, l->p ? l->p->pid : 0);
				edfyield();
			}
			if(i++ > 100000000){
				i = 0;
				lockloop(l, pc);
			}
		}
		if(up)
			inccnt(&up->nlocks);
		if(tas(&l->key) == 0){
			if(up)
				up->lastlock = l;
			l->pc = pc;
			l->p = up;
			l->isilock = 0;
			return 1;
		}
		if(up)
			deccnt(&up->nlocks);
	}
	return 0;	/* For the compiler */
}

void
ilock(Lock *l)
{
	ulong x;
	ulong pc;

	pc = getcallerpc(&l);
	lockstats.locks++;

	x = splhi();
	if(tas(&l->key) == 0){
		m->ilockdepth++;
		if(up)
			up->lastilock = l;
		l->sr = x;
		l->pc = pc;
		l->p = up;
		l->isilock = 1;
		return;
	}

	lockstats.glare++;
	if(conf.nmach < 2){
		dumplockmem("ilock:", l);
		panic("ilock: no way out: pc %luX\n", pc);
	}

	for(;;){
		lockstats.inglare++;
		splx(x);
		while(l->key)
			;
		x = splhi();
		if(tas(&l->key) == 0){
			m->ilockdepth++;
			if(up)
				up->lastilock = l;
			l->sr = x;
			l->pc = pc;
			l->p = up;
			l->isilock = 1;
			return;
		}
	}
}

int
canlock(Lock *l)
{
	if(up)
		inccnt(&up->nlocks);
	if(tas(&l->key)){
		if(up)
			deccnt(&up->nlocks);
		return 0;
	}

	if(up)
		up->lastlock = l;
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
	if(l->p != up)
		print("unlock: up changed: pc %lux, acquired at pc %lux, lock p 0x%p, unlock up 0x%p\n", getcallerpc(&l), l->pc, l->p, up);
	l->key = 0;
	coherence();

	if(up)
		deccnt(&up->nlocks);
}

void
iunlock(Lock *l)
{
	ulong sr;

	if(l->key == 0)
		print("iunlock: not locked: pc %luX\n", getcallerpc(&l));
	if(!l->isilock)
		print("iunlock of lock: pc %lux, held by %lux\n", getcallerpc(&l), l->pc);
	if(islo())
		print("iunlock while lo: pc %lux, held by %lux\n", getcallerpc(&l), l->pc);

	sr = l->sr;
	l->key = 0;
	coherence();

	m->splpc = getcallerpc(&l);
	m->ilockdepth--;
	if(up)
		up->lastilock = nil;
	splxpc(sr);
}
