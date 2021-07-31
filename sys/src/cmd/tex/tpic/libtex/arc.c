#include <math.h>
#include "tex.h"

devarc(x1, y1, x2, y2, xc, yc, r)
double x1, y1, x2, y2, xc, yc, r;
{
	double t, start, stop;
	int rad;

	/* tpic arcs go clockwise, and angles are measured clockwise */
	start = atan2(y2-yc, x2-xc);
	stop = atan2(y1-yc, x1-xc);
	if (r<0) {
		t = start; start = stop; stop = start;
	}
	rad = SCX(sqrt((x1-xc)*(x1-xc)+(y1-yc)*(y1-yc)));
	fprintf(TEXFILE, "    \\special{ar %d %d %d %d %6.3f %6.3f}%%\n",
		TRX(xc), TRY(yc), rad, rad, -start, -stop);
}
