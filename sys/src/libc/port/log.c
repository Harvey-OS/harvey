/*
	log returns the natural logarithm of its floating
	point argument.

	The coefficients are #2705 from Hart & Cheney. (19.38D)

	It calls frexp.
*/

#include <u.h>
#include <libc.h>

#define	log2    0.693147180559945309e0
#define	ln10o1  .4342944819032518276511
#define	sqrto2  0.707106781186547524e0
#define	p0      -.240139179559210510e2
#define	p1      0.309572928215376501e2
#define	p2      -.963769093377840513e1
#define	p3      0.421087371217979714e0
#define	q0      -.120069589779605255e2
#define	q1      0.194809660700889731e2
#define	q2      -.891110902798312337e1

double
log(double arg)
{
	double x, z, zsq, temp;
	int exp;

	if(arg <= 0)
		return NaN();
	x = frexp(arg, &exp);
	while(x < 0.5) {
		x *= 2;
		exp--;
	}
	if(x < sqrto2) {
		x *= 2;
		exp--;
	}

	z = (x-1) / (x+1);
	zsq = z*z;

	temp = ((p3*zsq + p2)*zsq + p1)*zsq + p0;
	temp = temp/(((zsq + q2)*zsq + q1)*zsq + q0);
	temp = temp*z + exp*log2;
	return temp;
}

double
log10(double arg)
{

	if(arg <= 0)
		return NaN();
	return log(arg) * ln10o1;
}
