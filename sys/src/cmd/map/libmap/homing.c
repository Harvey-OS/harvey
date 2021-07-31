#include "map.h"

static struct coord p0;		/* standard parallel */

static double
trigclamp(double x)
{
	return x>1? 1: x<-1? -1: x;
}

static struct coord az;		/* azimuth of p0 seen from place */
static struct coord rad;	/* angular dist from place to p0 */

static int
azimuth(struct place *place)
{
	if(place->nlat.c < .01)
		return 0;
	rad.c = trigclamp(p0.s*place->nlat.s +	/* law of cosines */
		p0.c*place->nlat.c*place->wlon.c);
	rad.s = sqrt(1 - rad.c*rad.c);
	if(fabs(rad.s) < .001) {
		az.s = 0;
		az.c = 1;
	} else {
		az.s = trigclamp(p0.c*place->wlon.s/rad.s); /* sines */
		az.c = trigclamp((p0.s - rad.c*place->nlat.s)
				/(rad.s*place->nlat.c));
	}
	rad.l = atan2(rad.s, rad.c);
	return 1;
}

static int
Xmecca(struct place *place, float *x, float *y)
{
	if(!azimuth(place))
		return 0;
	*x = -place->wlon.l;
	*y = fabs(az.s)<.02? -az.c*rad.s/p0.c: *x*az.c/az.s;
	return fabs(*y)>2? 0:
	       rad.c<0? -1:
	       1;
}

proj
mecca(float par)
{
	if(fabs(par)>80.)
		return(0);
	deg2rad(par,&p0);
	return(Xmecca);
}

static int
Xhoming(struct place *place, float *x, float *y)
{
	if(!azimuth(place))
		return 0;
	*x = -rad.l*az.s;
	*y = -rad.l*az.c;
	return place->wlon.c<0? -1: 1;
}

proj
homing(float par)
{
	if(fabs(par)>80.)
		return(0);
	deg2rad(par,&p0);
	return(Xhoming);
}
