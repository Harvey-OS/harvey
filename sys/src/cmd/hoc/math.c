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

#include "hoc.h"

double	errcheck(double, char*);

double
Log(double x)
{
	return errcheck(log(x), "log");
}
double
Log10(double x)
{
	return errcheck(log10(x), "log10");
}

double
Sqrt(double x)
{
	return errcheck(sqrt(x), "sqrt");
}

double
Exp(double x)
{
	return errcheck(exp(x), "exp");
}

double
Asin(double x)
{
	return errcheck(asin(x), "asin");
}

double
Acos(double x)
{
	return errcheck(acos(x), "acos");
}

double
Sinh(double x)
{
	return errcheck(sinh(x), "sinh");
}
double
Cosh(double x)
{
	return errcheck(cosh(x), "cosh");
}
double
Pow(double x, double y)
{
	return errcheck(pow(x,y), "exponentiation");
}

double
integer(double x)
{
	if(x<-2147483648.0 || x>2147483647.0)
		execerror("argument out of domain", 0);
	return (double)(long)x;
}

double
errcheck(double d, char* s)	/* check result of library call */
{
	if(isNaN(d))
		execerror(s, "argument out of domain");
	if(isInf(d, 0))
		execerror(s, "result out of range");
	return d;
}
