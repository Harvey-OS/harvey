#include <u.h>
#include <libc.h>

#define	Bias	128
#define	Emask	0xff
#define	Mbits	23
#define Mmask	((1<<Mbits)-1)
#define Mshift	8

typedef union
{
	double	d;
	long	v;
}Cheat;

double
modf(double d, double *ip)
{
	Cheat c;
	double x;
	long v;
	int e;

	if(d < 0)
		x = -d;
	else
		x = d;
	c.d = x;
	v = c.v;
	e = (v & Emask) - Bias;
	if(e < 0){
		*ip = 0;
		return d;
	}
	if(e < Mbits)
		v &= ~((Mmask >> e) << Mshift);
	c.v = v;
	x = c.d;
	if(d < 0)
		x = -x;
	*ip = x;
	return d - x;
}
