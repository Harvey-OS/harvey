/*
	C program for floating point sin/cos.
	Calls modf.
	There are no error exits.
	Coefficients are #3370 from Hart & Cheney (18.80D).
*/

#include <u.h>
#include <libc.h>

#define p0      .1357884097877375669092680e8
#define p1     -.4942908100902844161158627e7
#define p2      .4401030535375266501944918e6
#define p3     -.1384727249982452873054457e5
#define p4      .1459688406665768722226959e3
#define q0      .8644558652922534429915149e7
#define q1      .4081792252343299749395779e6
#define q2      .9463096101538208180571257e4
#define q3      .1326534908786136358911494e3

static
double
sinus(double arg, int quad)
{
	double e, f, ysq, x, y, temp1, temp2;
	int k;

	x = arg;
	if(x < 0) {
		x = -x;
		quad += 2;
	}
	x *= 1/PIO2;	/* underflow? */
	if(x > 32764) {
		y = modf(x, &e);
		e += quad;
		modf(0.25*e, &f);
		quad = e - 4*f;
	} else {
		k = x;
		y = x - k;
		quad += k;
		quad &= 3;
	}
	if(quad & 1)
		y = 1-y;
	if(quad > 1)
		y = -y;

	ysq = y*y;
	temp1 = ((((p4*ysq+p3)*ysq+p2)*ysq+p1)*ysq+p0)*y;
	temp2 = ((((ysq+q3)*ysq+q2)*ysq+q1)*ysq+q0);
	return temp1/temp2;
}

double
cos(double arg)
{
	if(arg < 0)
		arg = -arg;
	return sinus(arg, 1);
}

double
sin(double arg)
{
	return sinus(arg, 0);
}
