#include <u.h>
#include <libc.h>

/*
 * no nans: just return 0
 * no inf: return max floating value
 */
#define	NANEXP	(2047<<20)
#define	NANMASK	(2047<<20)
#define	NANSIGN	(1<<31)

double
NaN(void)
{
	return 0.0;
}

int
isNaN(double d)
{
	USED(d);
	return 0;
}

double
Inf(int sign)
{
	union
	{
		double	d;
		long	x;
	} a;

	if(sign < 0)
		a.x = 0x800000ff;
	else
		a.x = 0x7fffffff;
	return a.d;
}

int
isInf(double d, int sign)
{
	USED(d, sign);
	return 0;
}
