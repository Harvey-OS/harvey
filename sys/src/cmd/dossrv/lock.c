#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

void
mlock(MLock *l)
{

	if(l->key != 0 && l->key != 1)
		panic("uninitialized lock");
	if (l->key)
		panic("lock");
	l->key = 1;
}

void
unmlock(MLock *l)
{

	if(l->key != 0 && l->key != 1)
		panic("uninitialized unlock");
	if (!l->key)
		panic("unlock");
	l->key = 0;
}

int
canmlock(MLock *l)
{
	if(l->key != 0 && l->key != 1)
		panic("uninitialized canlock");
	if (l->key)
		return 0;
	l->key = 1;
	return 1;
}
