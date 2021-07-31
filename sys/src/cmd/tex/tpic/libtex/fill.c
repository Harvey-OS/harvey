#include "tex.h"
fill(num, ff)
int num[];
double *ff[];
{
	double *xp, *yp, **fp, x0, y0;
	int i, *n;
	n = num;
	fp = ff;
	while((i = *n++)){
		xp = *fp++;
		yp = xp+1;
		x0 = *xp;
		y0 = *yp;
		move(x0, y0);
		while(--i){
			xp += 2;
			yp += 2;
			vec(*xp, *yp);
		}
		if (*(xp-2) != x0 || *(yp-2) != y0)
			vec(x0, y0);
	}
}
