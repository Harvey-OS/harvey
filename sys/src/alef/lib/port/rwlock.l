#include <alef.h>

void
RWlock.Rlock(RWlock *l)
{
	l->x.lock();
	l->lock();
	l->readers++;
	l->k.canlock();
	l->unlock();
	l->x.unlock();
}

void
RWlock.Runlock(RWlock *l)
{
	l->lock();
	if(--l->readers == 0)
		l->k.unlock();
	l->unlock();
}

void
RWlock.Wlock(RWlock *l)
{
	l->x.lock();
	l->k.lock();
}

void
RWlock.Wunlock(RWlock *l)
{
	l->x.unlock();
	l->k.unlock();
}
