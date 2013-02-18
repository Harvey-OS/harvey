#include <u.h>
#include <libc.h>

ulong
ntruerand(ulong n)
{
	ulong m, r;

	/*
	 * set m to the one less than the maximum multiple of n <= 2^32,
	 * so we want a random number <= m.
	 */
	if(n > (1UL<<31))
		m = n-1;
	else
		/* 2^32 - 2^32%n - 1 = (2^32 - 1) - (2*(2^31%n))%n */
		m = 0xFFFFFFFFUL - (2*((1UL<<31)%n))%n;

	while((r = truerand()) > m)
		;

	return r%n;
}
