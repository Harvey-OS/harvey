/*ident	"@(#)cls4:incl-master/const-headers/complex.h	1.1" */
/*******************************************************************************
 
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1991 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/

#ifndef COMPLEXH
#define COMPLEXH
#pragma lib "ape/libap.a"
#pragma lib "c++/libC.a"

#include <iostream.h>
#include <errno.h>
#include <math.h>

#ifndef DOMAIN
#define DOMAIN		1
#endif
#ifndef SING
#define SING		2
#endif
#ifndef OVERFLOW
#define OVERFLOW	3
#endif
#ifndef UNDERFLOW
#define UNDERFLOW	4
#endif
#ifndef TLOSS
#define TLOSS		5
#endif
#ifndef PLOSS
#define PLOSS		6
#endif
#ifndef M_E
#define M_E	2.7182818284590452354
#endif
#ifndef M_LOG2E
#define M_LOG2E	1.4426950408889634074
#endif
#ifndef M_LOG10E
#define M_LOG10E	0.43429448190325182765
#endif
#ifndef M_LN2
#define M_LN2	0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10	2.30258509299404568402
#endif
#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2	1.57079632679489661923
#endif
#ifndef M_PI_4
#define M_PI_4	0.78539816339744830962
#endif
#ifndef M_1_PI
#define M_1_PI	0.31830988618379067154
#endif
#ifndef M_2_PI
#define M_2_PI	0.63661977236758134308
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI	1.12837916709551257390
#endif
#ifndef M_SQRT2
#define M_SQRT2	1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2	0.70710678118654752440
#endif

class complex {
	double	re, im;
public:
	complex()	{ re=0.0; im=0.0; }
	complex(double r, double i = 0.0)	{ re=r; im=i; }
	 	
	friend	double	real(const complex&);
	friend	double	imag(const complex&);

	friend	double	abs(complex);
	friend  double  norm(complex);
	friend  double	arg(complex);
	friend  complex conj(complex);
	friend  complex cos(complex);
	friend  complex cosh(complex);
	friend	complex exp(complex);
	friend  complex log(complex);
	friend  complex pow(double, complex);
	friend	complex pow(complex, int);
	friend	complex pow(complex, double);
	friend	complex pow(complex, complex);
	friend  complex	polar(double, double = 0);
	friend  complex sin(complex);
	friend  complex sinh(complex);
	friend	complex sqrt(complex);

	friend	complex	operator+(complex, complex);
	friend	complex	operator-(complex);
	friend	complex operator-(complex, complex);
	friend	complex operator*(complex, complex);
	friend 	complex operator/(complex, complex);
	friend 	int	operator==(complex, complex);
	friend 	int	operator!=(complex, complex);
	
	void operator+=(complex);
	void operator-=(complex);
	void operator*=(complex);
	void operator/=(complex);
};

ostream& operator<<(ostream&, complex);
istream& operator>>(istream&, complex&);

extern int errno;

inline double real(const complex& a)
{
	return a.re;
}

inline double imag(const complex& a)
{
	return a.im;
}

inline complex operator+(complex a1, complex a2) 
{
	return complex(a1.re+a2.re, a1.im+a2.im);
}

inline complex operator-(complex a1,complex a2) 
{
	return complex(a1.re-a2.re, a1.im-a2.im);
}

inline complex operator-(complex a) 
{
	return complex(-a.re, -a.im);
}

inline complex conj(complex a) 
{
	return complex(a.re, -a.im);
}

inline int operator==(complex a, complex b) 
{
	return (a.re==b.re && a.im==b.im);
}

inline int operator!=(complex a, complex b)
{
	return (a.re!=b.re || a.im!=b.im);
}

inline void complex::operator+=(complex a)
{
	re += a.re;
	im += a.im;
}

inline void complex::operator-=(complex a)
{
	re -= a.re;
	im -= a.im;
}


static const complex complex_zero(0,0);

class c_exception
{
	int	type;
	char	*name;
	complex	arg1;
	complex	arg2;
	complex	retval;
public:

	c_exception( char *n, const complex& a1, const complex& a2 = complex_zero )
		{ name = n; arg1 = a1; arg2 = a2; type = 0; }

	friend int complex_error( c_exception& );

	friend complex exp( complex );
	friend complex sinh( complex );
	friend complex cosh( complex );
	friend complex log( complex );	
};

#endif
