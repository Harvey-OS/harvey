#include <u.h>
#include <libc.h>

/*
 * this is big/little endian non-portable
 * it gets the endian from the FPdbleword
 * union in u.h.
 */
#define	MASK	0x7ffL
#define	SHIFT	20
#define	BIAS	1022L

double
frexp(double d, int *ep)
{
	FPdbleword x;

	if(d == 0) {
		*ep = 0;
		return 0;
	}
	x.x = d;
	*ep = ((x.hi >> SHIFT) & MASK) - BIAS;
	x.hi &= ~(MASK << SHIFT);
	x.hi |= BIAS << SHIFT;
	return x.x;
}

double
ldexp(double d, int e)
{
	FPdbleword x;

	if(d == 0)
		return 0;
	x.x = d;
	e += (x.hi >> SHIFT) & MASK;
	if(e <= 0)
		return 0;	/* underflow */
	if(e >= MASK){		/* overflow */
		if(d < 0)
			return Inf(-1);
		return Inf(1);
	}
	x.hi &= ~(MASK << SHIFT);
	x.hi |= (long)e << SHIFT;
	return x.x;
}

double
modf(double d, double *ip)
{
	FPdbleword x;
	int e;

	if(d < 1) {
		if(d < 0) {
			x.x = modf(-d, ip);
			*ip = -*ip;
			return -x.x;
		}
		*ip = 0;
		return d;
	}
	x.x = d;
	e = ((x.hi >> SHIFT) & MASK) - BIAS;
	if(e <= SHIFT+1) {
		x.hi &= ~(0x1fffffL >> e);
		x.lo = 0;
	} else
	if(e <= SHIFT+33)
		x.lo &= ~(0x7fffffffL >> (e-SHIFT-2));
	*ip = x.x;
	return d - x.x;
}
