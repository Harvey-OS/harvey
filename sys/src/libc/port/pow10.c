#include	<u.h>
#include	<libc.h>

static
long	tab[] =
{
	1L,
	10L,
	100L,
	1000L,
	10000L,
	100000L,
	1000000L,
	10000000L,
	100000000L,
	1000000000L,
};

double
pow10(int n)
{
	int m;

	if(n < 0)
		return 1/pow10(-n);
	if(n < sizeof(tab)/sizeof(tab[0]))
		return tab[n];
	m = n/2;
	return pow10(m) * pow10(n-m);
}
