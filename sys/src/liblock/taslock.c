#include <u.h>
#include <libc.h>
#include <lock.h>

int	tas(int*);

void
lockinit(void)
{
}

void
lock(Lock *lk)
{
	while(tas(&lk->val))
		sleep(0);
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
