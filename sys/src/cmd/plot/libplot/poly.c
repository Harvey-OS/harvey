/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "mplot.h"
void plotpoly(int num[], double *ff[]){
	double *xp, *yp, **fp;
	int i, *n;
	n = num;
	fp = ff;
	while((i = *n++)){
		xp = *fp++;
		yp = xp+1;
		move(*xp, *yp);
		while(--i){
			xp += 2;
			yp += 2;
			vec(*xp, *yp);
		}
	}
}
