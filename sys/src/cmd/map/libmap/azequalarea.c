#include "map.h"

int
Xazequalarea(struct place *place, float *x, float *y)
{
	double r;
	r = sqrt(1. - place->nlat.s);
	*x = - r * place->wlon.s;
	*y = - r * place->wlon.c;
	return(1);
}

proj
azequalarea(void)
{
	return(Xazequalarea);
}
