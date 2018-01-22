/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "map.h"

int Xstereographic(struct place *place, double *x, double *y);

static struct place eastpole;
static struct place westpole;
static double eastx, easty;
static double westx, westy;
static double scale;
static double pwr;

/* conformal map w = ((1+z)^A - (1-z)^A)/((1+z)^A + (1-z)^A),
   where A<1, maps unit circle onto a convex lune with x= +-1
   mapping to vertices of angle A*PI at w = +-1 */

/* there are cuts from E and W poles to S pole,
   in absence of a cut routine, error is returned for
   points outside a polar cap through E and W poles */

static Xlune(struct place *place, double *x, double *y)
{
	double stereox, stereoy;
	double z1x, z1y, z2x, z2y;
	double w1x, w1y, w2x, w2y;
	double numx, numy, denx, deny;
	if(place->nlat.l < eastpole.nlat.l-FUZZ)
		return	-1;
	Xstereographic(place, &stereox, &stereoy);
	stereox *= scale;
	stereoy *= scale;
	z1x = 1 + stereox;
	z1y = stereoy;
	z2x = 1 - stereox;
	z2y = -stereoy;
	cpow(z1x,z1y,&w1x,&w1y,pwr);
	cpow(z2x,z2y,&w2x,&w2y,pwr);
	numx = w1x - w2x;
	numy = w1y - w2y;
	denx = w1x + w2x;
	deny = w1y + w2y;
	cdiv(numx, numy, denx, deny, x, y);
	return 1;
}

proj
lune(double lat, double theta)
{
	deg2rad(lat, &eastpole.nlat);
	deg2rad(-90.,&eastpole.wlon);
	deg2rad(lat, &westpole.nlat);
	deg2rad(90. ,&westpole.wlon);
	Xstereographic(&eastpole, &eastx, &easty);
	Xstereographic(&westpole, &westx, &westy);
	if(fabs(easty)>FUZZ || fabs(westy)>FUZZ ||
	   fabs(eastx+westx)>FUZZ)
		abort();
	scale = 1/eastx;
	pwr = theta/180;
	return Xlune;
}
