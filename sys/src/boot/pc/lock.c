#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#define PCOFF -1

/*
 */
void
lock(Lock *l)
{
	int i;

	/*
	 * Try the fast grab first
	 */
    	if (tas(l) == 0){
		if(u && u->p)
			u->p->hasspin = 1;
		l->pc = ((ulong*)&l)[PCOFF];
		return;
	}
	for (i = 0; i < 1000; i++) {
		if (tas(l) == 0){
			if(u && u->p)
				u->p->hasspin = 1;
			l->pc = ((ulong*)&l)[PCOFF];
			return;
		}
	}
	l->key = 0;
	panic("lock loop 0x%lux 0x%lux pc 0x%lux held by pc 0x%lux\n", l, l->key,
		((ulong*)&l)[PCOFF], l->pc);
}

int
canlock(Lock *l)
{
	if(tas(l))
		return 0;
	if(u && u->p)
		u->p->hasspin = 1;
	return 1;
}

void
unlock(Lock *l)
{
	l->key = 0;
	l->pc = 0;
	if(u && u->p)
		u->p->hasspin = 0;
}
