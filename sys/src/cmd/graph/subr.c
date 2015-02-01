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
#include <stdio.h>
#include "iplot.h"
void putnum(int num[], double *ff[]){
	double **fp, *xp;
	int *np, n,i;
	np = num;
	fp = ff;
	while( (n = *np++)){
		xp = *fp++;
		printf("{ ");
		for(i=0; i<n;i++){
			printf("%g %g ",*xp, *(xp+1));
			if(i&1)printf("\n");
		}
		printf("}\n");
	}
}
