#include "include.h"

void
lock(Lock* l)
{
	for(;;){
		while(l->key)
			;
		if(TAS(&l->key) == 0)
			return;
	}
}

void
unlock(Lock* l)
{
	l->key = 0;
}
