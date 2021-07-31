#include "map.h"

static float scale;

static int
Xgall(struct place *place, float *x, float *y)
{
	/* two ways to compute tan(place->nlat.l/2) */
	if(fabs(place->nlat.s)<.1)
		*y = sin(place->nlat.l/2)/cos(place->nlat.l/2);
	else
		*y = (1-place->nlat.c)/place->nlat.s;
	*x = -scale*place->wlon.l;
	return 1;
}

proj
gall(float par)
{
	float coshalf;
	if(fabs(par)>80)
		return 0;
	par *= RAD;
	coshalf = cos(par/2);
	scale = cos(par)/(2*coshalf*coshalf);
	return Xgall;
}
