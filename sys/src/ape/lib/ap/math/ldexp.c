#include <math.h>
#include <errno.h>
#define _RESEARCH_SOURCE
#include <float.h>

/* ldexp suitable for IEEE double-precision */

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
	} i;
} Cheat;

double
ldexp(double d, int e)
{
	Cheat x;
	int de;

	if(d == 0)
		return 0;
	x.d = d;
	de = (x.i.ms >> SHIFT) & MASK;
	e += de;
	if(e <= 0 || de == 0){
		errno = ERANGE;
		return 0;
	}
	if(e >= MASK || de == MASK){
		errno = ERANGE;
		if(d < 0)
			return -HUGE_VAL;
		return HUGE_VAL;
	}
	x.i.ms &= ~(MASK << SHIFT);
	x.i.ms |= (long)e << SHIFT;
	return x.d;
}
