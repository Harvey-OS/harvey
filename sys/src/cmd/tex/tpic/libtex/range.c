#include "tex.h"
range(x0, y0, x1, y1) 
double	x0, y0, x1, y1; 
{
	e1->xmin = x0;
	e1->ymin = y0;
	if (x1-x0 < .0000001*e1->sidex)
		x1=x0+.0000001;
	if (y1-y0 < .0000001*e1->sidey)
		y1=y0+.0000001;
	e1->scalex = e0->scalex*e1->sidex / (x1 - x0);
	e1->scaley = e0->scaley*e1->sidey / (y1 - y0);
}
