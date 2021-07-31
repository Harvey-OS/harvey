#include "map.h"

static struct coord stdpar0, stdpar1;
static double k;
static double yeq;

static int
Xortelius(struct place *place, double *x, double *y)
{
	*y = yeq + fabs(place->nlat.l);
	*x = *y*k*place->wlon.l;
	if(place->nlat.l < 0)
		*y = 2*yeq - *y;
	return 1;
}

proj
ortelius(double par0, double par1)
{
	par0 = fabs(par0);
	par1 = fabs(par1);
	deg2rad(par0,&stdpar0);
	deg2rad(par1,&stdpar1);
	if(fabs(par1-par0) < .1)
		k = stdpar1.s;
	else
		k = (stdpar1.c-stdpar0.c)/(stdpar0.l-stdpar1.l);
	if(k < .1)
		return rectangular(0);
	yeq = -stdpar1.l - stdpar1.c/k;
	return Xortelius;
}
