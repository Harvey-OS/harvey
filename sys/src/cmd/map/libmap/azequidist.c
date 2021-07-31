#include "map.h"

int
Xazequidistant(struct place *place, float *x, float *y)
{
	float colat;
	colat = PI/2 - place->nlat.l;
	*x = -colat * place->wlon.s;
	*y = -colat * place->wlon.c;
	return(1);
}

proj
azequidistant(void)
{
	return(Xazequidistant);
}
