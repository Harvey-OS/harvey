#include "map.h"
/* refractive fisheye, not logarithmic */

static float n;

static int
Xfisheye(struct place *place, float *x, float *y)
{
	float r;
	float u = sin(PI/4-place->nlat.l/2)/n;
	if(fabs(u) > .97)
		return -1;
	r = tan(asin(u));
	*x = -r*place->wlon.s;
	*y = -r*place->wlon.c;
	return 1;
}

proj
fisheye(float par)
{
	n = par;
	return n<.1? 0: Xfisheye;
}
