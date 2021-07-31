#include <u.h>
#include <libc.h>

#define	Bias	128
#define Emask	0xff
#define Sign	0x80000000

typedef union
{
	double	d;
	long	v;
}Cheat;

double
frexp(double d, int *exp)
{
	Cheat c;
	long v;
	int e;

	c.d = d;
	v = c.v;
	e = v & Emask;
	if(e == 0){
		*exp = 0;
		return 0.0;
	}
	v &= ~Emask;
	if(v == Sign){
		/*
		 * stupid negative powers of 2
		 */
		*exp = e - (Bias-2);
		v |= Bias - 2;
	}else{
		*exp = e - (Bias-1);
		v |= Bias - 1;
	}
	c.v = v;
	return c.d;
}
