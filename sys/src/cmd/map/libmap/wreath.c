#include "map.h"

/* christmas wreath projection, 1989 card.
   wreath par1 par2
   par1 is fraction of full circle the wreath is to cover
	(the gap is where you put the bow)
   par2 tells how deeply to curve the meridians, about .2 - .5
   inner radius is 1-a, equatorial is 1, outer is 1+a
   
   fudging for beauty: shift things south 30 degrees so
   equator is really not at radius 1, and stretch northern
   hemisphere quadratically away from the equator
*/
static float a = .7, b, c;
int squinge;

static int (*Xcylequalarea)(struct place *, float *, float *);

static int
Xwreath(struct place *place, float *x, float *y)
{
	float xx, yy, r, h, theta;
	struct place nplace;
	float l;
	nplace = *place;
	if(squinge) {
		if(nplace.nlat.l < 0)
			nplace.nlat.l -= PI/6-FUZZ;
		else {
			l = nplace.nlat.l;
			nplace.nlat.l = -PI/6 + l + 2*l*l/(3*PI);
		}
		if(nplace.nlat.l < -PI/2)
			return 0;
		sincos(&nplace.nlat);
	}
	Xcylequalarea(&nplace, &xx, &yy);
	r = 1 - a*yy;
	theta = b*xx;
	h = c*sin(theta)*sqrt(1-yy*yy);
	theta += h/r;
	*y = -r*cos(theta);
	*x = r*sin(theta);
	return(1);
}

proj
wreath(float par1, float par2)
{
	Xcylequalarea = cylequalarea(45.);
	b = fabs(par1);
	c = par2;
	squinge = par1<0;
	return Xwreath;
}
