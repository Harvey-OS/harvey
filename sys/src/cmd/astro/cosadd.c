/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "astro.h"


void
icosadd(double *fp, char *cp)
{

	cafp = fp;
	cacp = cp;
}

double
cosadd(int n, double coef, ...)
{
	double *coefp;
	char *cp;
	int i;
	double sum, a1, a2;

	sum = 0;
	cp = cacp;

loop:
	a1 = *cafp++;
	if(a1 == 0) {
		cacp = cp;
		return sum;
	}
	a2 = *cafp++;
	i = n;
	coefp = &coef;
	do
		a2 += *cp++ * *coefp++;
	while(--i);
	sum += a1 * cos(a2);
	goto loop;
}

double
sinadd(int n, double coef, ...)
{
	double *coefp;
	char *cp;
	int i;
	double sum, a1, a2;

	sum = 0;
	cp = cacp;

loop:
	a1 = *cafp++;
	if(a1 == 0) {
		cacp = cp;
		return sum;
	}
	a2 = *cafp++;
	i = n;
	coefp = &coef;
	do
		a2 += *cp++ * *coefp++;
	while(--i);
	sum += a1 * sin(a2);
	goto loop;
}
