#include <alef.h>

#define	HZ	1000.0

float
cputime(void)
{
	int i, t[4];

	times(t);
	for(i=1; i<4; i++)
		t[0] += t[i];
	return t[0] / HZ;
}
