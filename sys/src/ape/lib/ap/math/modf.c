#include <math.h>
#include <errno.h>

/* modf suitable for IEEE double-precision */

#define	MASK	0x7ffL
#define SIGN	0x80000000
#define	SHIFT	20
#define	BIAS	1022L

typedef	union
{
	double	d;
	struct
	{
		long	ms;
		long	ls;
	} i;
} Cheat;

double
modf(double d, double *ip)
{
	Cheat x;
	int e;

	if(-1 < d && d < 1) {
		*ip = 0;
		return d;
	}
	x.d = d;
	x.i.ms &= ~SIGN;
	e = (x.i.ms >> SHIFT) & MASK;
	if(e == MASK || e == 0){
		errno = EDOM;
		*ip = (d > 0)? HUGE_VAL : -HUGE_VAL;
		return 0;
	}
	e -= BIAS;
	if(e <= SHIFT+1) {
		x.i.ms &= ~(0x1fffffL >> e);
		x.i.ls = 0;
	} else
	if(e <= SHIFT+33)
		x.i.ls &= ~(0x7fffffffL >> (e-SHIFT-2));
	if(d > 0){
		*ip = x.d;
		return d - x.d;
	}else{
		*ip = -x.d;
		return d + x.d;
	}
}
