#include "map.h"

static float viewpt;

static int
Xperspective(struct place *place, float *x, float *y)
{
	float r;
	if(viewpt<=1. && place->nlat.s<=viewpt+.01)
		return(-1);
	r = place->nlat.c*(viewpt - 1.)/(viewpt - place->nlat.s);
	*x = - r*place->wlon.s;
	*y = - r*place->wlon.c;
	if(r>4.)
		return(0);
	if(fabs(viewpt)>1. && place->nlat.s<=1./viewpt)
		return(0);
	return(1);
}

int
Xstereographic(struct place *place, float *x, float *y)
{
	viewpt = -1;
	return(Xperspective(place, x, y));
}

proj
perspective(float radius)
{
	viewpt = radius;
	if(radius>=1000.)
		return(Xorthographic);
	if(fabs(radius-1.)<.0001)
		return(0);
	return(Xperspective);
}

proj
stereographic(void)
{
	viewpt = -1.;
	return(Xperspective);
}

proj
gnomonic(void)
{
	viewpt = 0.;
	return(Xperspective);
}
