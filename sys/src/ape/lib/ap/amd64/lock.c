#define _LOCK_EXTENSION
#include "../plan9/sys9.h"
#include <lock.h>

int	tas(int*);

void
lock(Lock *lk)
{
	while(tas(&lk->val))
		_SLEEP(0);
}

int
canlock(Lock *lk)
{
	if(tas(&lk->val))
		return 0;
	return 1;
}

void
unlock(Lock *lk)
{
	lk->val = 0;
}
