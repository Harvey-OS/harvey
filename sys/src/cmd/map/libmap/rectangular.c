#include "map.h"

static float scale;

static int
Xrectangular(struct place *place, float *x, float *y)
{
	*x = -scale*place->wlon.l;
	*y = place->nlat.l;
	return(1);
}

proj
rectangular(float par)
{
	scale = cos(par*RAD);
	if(scale<.1)
		return 0;
	return(Xrectangular);
}
