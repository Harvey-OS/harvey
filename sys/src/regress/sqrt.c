/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

/*
	sqrtC returns the square root of its floating
	point argument. Newton's method.

	calls frexp
*/
double
sqrtC(double arg)
{
	double x, temp;
	int exp, i;

	if(arg <= 0) {
		if(arg < 0)
			return NaN();
		return 0;
	}
	if(isInf(arg, 1))
		return arg;
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

void
main(void)
{
	double v;
	char *a, *b;
	
	for(v = 4; v < 65536; v += 3) {
		a = smprint("%f", sqrtC(v));
		b = smprint("%f", sqrt(v));
		if(strcmp(a, b)){
			print("FAIL");
			exits("FAIL");
		}
		free(a);
		free(b);
	}
	
	print("PASS");
	exits("PASS");
}
