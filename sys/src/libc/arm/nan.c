#include <u.h>
#include <libc.h>

#define	NANEXP	(2047<<20)
#define	NANMASK	(2047<<20)
#define	NANSIGN	(1<<31)

double
NaN(void)
{
	union
	{
		double	d;
		long	x[2];
	} a;

	a.x[0] = NANEXP;
	a.x[1] = 1;
	return a.d;
}

int
isNaN(double d)
{
	union
	{
		double	d;
		long	x[2];
	} a;

	a.d = d;
	if((a.x[0] & NANMASK) != NANEXP)
		return 0;
	return !isInf(d, 0);
}

double
Inf(int sign)
{
	union
	{
		double	d;
		long	x[2];
	} a;

	a.x[0] = NANEXP;
	a.x[1] = 0;
	if(sign < 0)
		a.x[0] |= NANSIGN;
	return a.d;
}

int
isInf(double d, int sign)
{
	union
	{
		double	d;
		long	x[2];
	} a;

	a.d = d;
	if(a.x[1] != 0)
		return 0;
	if(a.x[0] == NANEXP)
		return sign >= 0;
	if(a.x[0] == (NANEXP|NANSIGN))
		return sign <= 0;
	return 0;
}
