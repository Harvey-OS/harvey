#include <u.h>
#include <libc.h>

#define	HZ	1000

double
cputime(void)
{
	long t[4];
	long times(long*);
	int i;

	times(t);
	for(i=1; i<4; i++)
		t[0] += t[i];
	return t[0] / (double)HZ;
}
