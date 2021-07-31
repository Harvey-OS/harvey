#include <alef.h>

enum
{
	MASK	= 0x7ff,
	SHIFT	= 20,
	BIAS	= 1022,
};

union Cheat
{
	float	d;
	aggr
	{
		int	ls;
		int	ms;
	};
};

float
frexp(float d, int *ep)
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

float
ldexp(float d, int e)
{
	Cheat x;

	if(d == 0)
		return 0;
	x.d = d;
	e += (x.ms >> SHIFT) & MASK;
	if(e <= 0)
		return 0;	/* underflow */
	if(e >= MASK){		/* overflow */
		if(d < 0)
			return Inf(-1);
		return Inf(1);
	}
	x.ms &= ~(MASK << SHIFT);
	x.ms |= (int)e << SHIFT;
	return x.d;
}

float
modf(float d, float *ip)
{
	Cheat x;
	int e;

	if(d < 1) {
		if(d < 0) {
			x.d = modf(-d, ip);
			*ip = -*ip;
			return -x.d;
		}
		*ip = 0;
		return d;
	}
	x.d = d;
	e = ((x.ms >> SHIFT) & MASK) - BIAS;
	if(e <= SHIFT+1) {
		x.ms &= ~(0x1fffff >> e);
		x.ls = 0;
	} else
	if(e <= SHIFT+33)
		x.ls &= ~(0x7fffffff >> (e-SHIFT-2));
	*ip = x.d;
	return d - x.d;
}
