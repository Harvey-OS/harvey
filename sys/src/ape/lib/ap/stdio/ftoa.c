/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <math.h>
#include <stdlib.h>
double pow10(int);
#define	NDIG	18
#define	NFTOA	(NDIG+4)
/*
 * convert floating to ascii.  ftoa returns an integer e such that
 * f=g*10**e, with .1<=|g|<1 (e=0 when g==0) and puts an ascii
 * representation of g in the buffer pointed to by bp.  bp[0] will
 * be '+' or '-', and bp[1] to bp[NFTOA-2]
 * will be appropriate digits of g. bp[NFTOA-1] will be '\0'
 */
int ftoa(double f, char *bp){
	int e, e1, e2, i;
	double digit, g, p;
	if(f>=0) *bp++='+';
	else{
		f=-f;
		*bp++='-';
	}
	/* find e such that f==0 or 1<=f*pow10(e)<10, and set f=f*pow10(e) */
	if(f==0.) e=1;
	else{
		frexp(f, &e);
		e=-e*30103/100000;
		/* split in 2 pieces to guard against overflow in extreme cases */
		e1=e/2;
		e2=e-e1;
		p=f*pow10(e2);
		while((g=p*pow10(e1))<1.) e1++;
		while((g=p*pow10(e1))>=10.) --e1;
		e=e1+e2;
		f=g;
	}
	for(i=0;i!=NDIG;i++){
		f=modf(f, &digit)*10.;
		*bp++=digit+'0';
	}
	*bp='\0';
	return 1-e;
}
