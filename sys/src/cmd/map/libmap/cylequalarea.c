#include "map.h"

static float a;

static int
Xcylequalarea(struct place *place, float *x, float *y)
{
	*x = - place->wlon.l * a;
	*y = place->nlat.s;
	return(1);
}

proj
cylequalarea(float par)
{
	struct coord stdp0;
	if(par > 89.0)
		return(0);
	deg2rad(par, &stdp0);
	a = stdp0.c*stdp0.c;
	return(Xcylequalarea);
}
