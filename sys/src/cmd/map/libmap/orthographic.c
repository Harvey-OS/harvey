#include "map.h"

int
Xorthographic(struct place *place, float *x, float *y)
{
	*x = - place->nlat.c * place->wlon.s;
	*y = - place->nlat.c * place->wlon.c;
	return(place->nlat.l<0.? 0 : 1);
}

proj
orthographic(void)
{
	return(Xorthographic);
}
