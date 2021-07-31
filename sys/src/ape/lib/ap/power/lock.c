#include "../plan9/lib.h"
#include "../plan9/sys9.h"
#define _LOCK_EXTENSION
#include <lock.h>

int	tas(int*);

void
lock(Lock *lk)
{
	int i;

	/* once fast */
	if(!tas(&lk->val))
		return;
	/* a thousand times pretty fast */
	for(i=0; i<1000; i++){
		if(!tas(&lk->val))
			return;
		_SLEEP(0);
	}
	/* now nice and slow */
	for(i=0; i<1000; i++){
		if(!tas(&lk->val))
			return;
		_SLEEP(100);
	}
	/* take your time */
	while(tas(&lk->val))
		_SLEEP(1000);
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
