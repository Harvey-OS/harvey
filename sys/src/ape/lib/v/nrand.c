#include <stdlib.h>

#define	MASK	0x7FFFFFFFL
#define	FRACT	(1.0 / (MASK + 1.0))

extern long lrand(void);

double
frand(void)
{

	return lrand() * FRACT;
}

nrand(int n)
{
	long slop, v;

	slop = MASK % n;
	do
		v = lrand();
	while(v <= slop);
	return v % n;
}
