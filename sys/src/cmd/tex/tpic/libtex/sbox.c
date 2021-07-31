#include "tex.h"
sbox(x0, y0, x1, y1) 
double	x0, y0, x1, y1;
{
	fprintf(TEXFILE,"    \\special{bk}%%\n");
	box(x0, y0, x1, y1);
}
