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
#define	SIG	52

double
frexp(double d, int *ep)
{
	FPdbleword x, a;

	*ep = 0;
	/* order matters: only isNaN can operate on NaN */
	if(isNaN(d) || isInf(d, 0) || d == 0)
		return d;
	x.x = d;
	a.x = fabs(d);
	if((a.hi >> SHIFT) == 0){	/* normalize subnormal numbers */
		x.x = (double)(1ULL<<SIG) * d;
		*ep = -SIG;
	}
	*ep += ((x.hi >> SHIFT) & MASK) - BIAS;
	x.hi &= ~(MASK << SHIFT);
	x.hi |= BIAS << SHIFT;
	return x.x;
}

double
ldexp(double d, int deltae)
{
	int e, bits;
	FPdbleword x;
	ulong z;

	if(d == 0)
		return 0;
	x.x = d;
	e = (x.hi >> SHIFT) & MASK;
	if(deltae >= 0 || e+deltae >= 1){	/* no underflow */
		e += deltae;
		if(e >= MASK){		/* overflow */
			if(d < 0)
				return Inf(-1);
			return Inf(1);
		}
	}else{	/* underflow gracefully */
		deltae = -deltae;
		/* need to shift d right deltae */
		if(e > 1){		/* shift e-1 by exponent manipulation */
			deltae -= e-1;
			e = 1;
		}
		if(deltae > 0 && e==1){	/* shift 1 by switch from 1.fff to 0.1ff */
			deltae--;
			e = 0;
			x.lo >>= 1;
			x.lo |= (x.hi&1)<<31;
			z = x.hi & ((1<<SHIFT)-1);
			x.hi &= ~((1<<SHIFT)-1);
			x.hi |= (1<<(SHIFT-1)) | (z>>1);
		}
		while(deltae > 0){		/* shift bits down */
			bits = deltae;
			if(bits > SHIFT)
				bits = SHIFT;
			x.lo >>= bits;
			x.lo |= (x.hi&((1<<bits)-1)) << (32-bits);
			z = x.hi & ((1<<SHIFT)-1);
			x.hi &= ~((1<<SHIFT)-1);
			x.hi |= z>>bits;
			deltae -= bits;
		}
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

	x.x = d;
	e = (x.hi >> SHIFT) & MASK;
	if(e == MASK){
		*ip = d;
		if(x.lo != 0 || (x.hi & 0xfffffL) != 0)	/* NaN */
			return d;
		/* ±Inf */
		x.hi &= 0x80000000L;
		return x.x;
	}
	if(d < 1) {
		if(d < 0) {
			x.x = modf(-d, ip);
			*ip = -*ip;
			return -x.x;
		}
		*ip = 0;
		return d;
	}
	e -= BIAS;
	if(e <= SHIFT+1) {
		x.hi &= ~(0x1fffffL >> e);
		x.lo = 0;
	} else
	if(e <= SHIFT+33)
		x.lo &= ~(0x7fffffffL >> (e-SHIFT-2));
	*ip = x.x;
	return d - x.x;
}
