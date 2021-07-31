#include "map.h"

static double a;

static int
Xnewyorker(struct place *place, float *x, float *y)
{
	float r = PI/2 - place->nlat.l;
	float s;
	if(r<.001)	/* cheat to plot center */
		s = 0;
	else if(r<a)
		return -1;
	else
		s = log(r/a);
	*x = -s * place->wlon.s;
	*y = -s * place->wlon.c;
	return(1);
}

proj
newyorker(float a0)
{
	a = a0*RAD;
	return(Xnewyorker);
}
