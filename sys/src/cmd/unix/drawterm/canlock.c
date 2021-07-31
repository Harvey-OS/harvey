#include "lib9.h"

int
canlock(Lock *l)
{
	extern int tas(void*);

	return !tas(&l->val);
}
