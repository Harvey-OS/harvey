#include <math.h>
#include <errno.h>
#define _RESEARCH_SOURCE
#include <float.h>

#define	MASK	0x7ffL
#define	SHIFT	20
#define	BIAS	1022L

typedef	union
{
	double	d;
	struct
	{
#ifdef IEEE_8087
		long	ls;
		long	ms;
#else
		long	ms;
		long	ls;
#endif
	};
} Cheat;

double
frexp(double d, int *ep)
{
	Cheat x;

	if(d == 0) {
		*ep = 0;
		return 0;
	}
	x.d = d;
	*ep = ((x.ms >> SHIFT) & MASK) - BIAS;
	x.ms &= ~(MASK << SHIFT);
	x.ms |= BIAS << SHIFT;
	return x.d;
}

double
ldexp(double d, int e)
{
	Cheat x;

	if(d == 0)
		return 0;
	x.d = d;
	e += (x.ms >> SHIFT) & MASK;
	if(e <= 0)
		return 0;
	if(e >= MASK){
		errno = ERANGE;
		if(d < 0)
			return -HUGE_VAL;
		return HUGE_VAL;
	}
	x.ms &= ~(MASK << SHIFT);
	x.ms |= (long)e << SHIFT;
	return x.d;
}

double
modf(double d, double *ip)
{
	double f;
	Cheat x;
	int e;

	if(d < 1) {
		if(d < 0) {
			f = modf(-d, ip);
			*ip = -*ip;
			return -f;
		}
		*ip = 0;
		return d;
	}
	x.d = d;
	e = ((x.ms >> SHIFT) & MASK) - BIAS;
	if(e <= SHIFT+1) {
		x.ms &= ~(0x1fffffL >> e);
		x.ls = 0;
	} else
	if(e <= SHIFT+33)
		x.ls &= ~(0x7fffffffL >> (e-SHIFT-2));
	*ip = x.d;
	return d - x.d;
}
