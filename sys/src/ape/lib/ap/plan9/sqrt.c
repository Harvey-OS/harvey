/*
	sqrt returns the square root of its floating
	point argument. Newton's method.

	calls frexp
*/

#include <math.h>
#include <errno.h>
#define _RESEARCH_SOURCE
#include <float.h>

static double Inf(int);
static int isInf(double, int);

double
sqrt(double arg)
{
	double x, temp;
	int exp, i;

	if(isInf(arg, 1))
		return arg;
	if(arg <= 0) {
		if(arg < 0)
			errno = EDOM;
		return 0;
	}
	x = frexp(arg, &exp);
	while(x < 0.5) {
		x *= 2;
		exp--;
	}
	/*
	 * NOTE
	 * this wont work on 1's comp
	 */
	if(exp & 1) {
		x *= 2;
		exp--;
	}
	temp = 0.5 * (1.0+x);

	while(exp > 60) {
		temp *= (1L<<30);
		exp -= 60;
	}
	while(exp < -60) {
		temp /= (1L<<30);
		exp += 60;
	}
	if(exp >= 0)
		temp *= 1L << (exp/2);
	else
		temp /= 1L << (-exp/2);
	for(i=0; i<=4; i++)
		temp = 0.5*(temp + arg/temp);
	return temp;
}

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

#define	NANEXP	(2047<<20)
#define	NANMASK	(2047<<20)
#define	NANSIGN	(1<<31)

static double
Inf(int sign)
{
	Cheat a;

	a.ms = NANEXP;
	a.ls = 0;
	if(sign < 0)
		a.ms |= NANSIGN;
	return a.d;
}

static int
isInf(double d, int sign)
{
	Cheat a;

	a.d = d;
	if(a.ls != 0)
		return 0;
	if(a.ms == NANEXP)
		return sign >= 0;
	if(a.ms == (NANEXP|NANSIGN))
		return sign <= 0;
	return 0;
}
