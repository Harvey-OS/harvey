#include "map.h"

static int
Xmercator(struct place *place, float *x, float *y)
{
	if(fabs(place->nlat.l) > 80.*RAD)
		return(-1);
	*x = -place->wlon.l;
	*y = 0.5*log((1+place->nlat.s)/(1-place->nlat.s));
	return(1);
}

proj
mercator(void)
{
	return(Xmercator);
}

static float ecc = ECC;

static int
Xspmercator(struct place *place, float *x, float *y)
{
	if(Xmercator(place,x,y) < 0)
		return(-1);
	*y += 0.5*ecc*log((1-ecc*place->nlat.s)/(1+ecc*place->nlat.s));
	return(1);
}

proj
sp_mercator(void)
{
	return(Xspmercator);
}
