#include	<alef.h>

intern
int	tab[] =
{
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
};

float
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
