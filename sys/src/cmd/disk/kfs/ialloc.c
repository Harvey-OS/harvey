#include "all.h"

void *ialloc(ulong n){
	void *p;

	if(p = malloc(n))
		memset(p, 0, n);
	return p;
}
