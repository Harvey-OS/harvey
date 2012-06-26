#include <errno.h>
#include <float.h>
#include <math.h>

static	long	tab[] =
{
	1L, 10L, 100L, 1000L, 10000L,
	100000L, 1000000L, 10000000L
};

double
pow10(int n)
{
	int m;

	if(n > DBL_MAX_10_EXP){
		errno = ERANGE;
		return HUGE_VAL;
	}
	if(n < DBL_MIN_10_EXP){
		errno = ERANGE;
		return 0.0;
	}
	if(n < 0)
		return 1/pow10(-n);
	if(n < sizeof(tab)/sizeof(tab[0]))
		return tab[n];
	m = n/2;
	return pow10(m) * pow10(n-m);
}
