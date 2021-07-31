#include <u.h>
#include <libc.h>

#define	Bias	128
#define	Emask	0xff
#define Sign	0x80000000

typedef union
{
	double	d;
	long	v;
}Cheat;

double
ldexp(double d, int exp)
{
	Cheat c;
	long v;
	int e, min, max;

	c.d = d;
	v = c.v;
	e = v & Emask;
	if(e == 0)
		return 0.0;
	min = 1 - e;
	max = Emask - e;
	if(exp < min)
		return 0.0;
	if(exp > max){
		if(v & Sign)
			return Inf(-1);
		return Inf(1);
	}
	c.v = (v & ~Emask) | (e + exp);
	return c.d;
}
