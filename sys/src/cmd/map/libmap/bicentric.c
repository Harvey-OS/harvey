#include "map.h"

static struct coord center;

static int
Xbicentric(struct place *place, float *x, float *y)
{
	if(place->wlon.c<=.01||place->nlat.c<=.01)
		return(-1);
	*x = -center.c*place->wlon.s/place->wlon.c;
	*y = place->nlat.s/(place->nlat.c*place->wlon.c);
	return(*x**x+*y**y<=9);
}

proj
bicentric(float l)
{
	l = fabs(l);
	if(l>89)
		return(0);
	deg2rad(l,&center);
	return(Xbicentric);
}
