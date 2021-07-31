#include "all.h"

void
lock(Lock *l)
{
	if(l->key)
		panic("lock");
	l->key = 1;
}

void
unlock(Lock *l)
{
	if(!l->key)
		panic("unlock");
	l->key = 0;
}

int
canlock(Lock *l)
{
	if(l->key)
		return 0;
	l->key = 1;
	return 1;
}
