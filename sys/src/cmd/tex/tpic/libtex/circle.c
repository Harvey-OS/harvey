#include "tex.h"
circle(xc, yc, r)
double xc, yc, r;
{
	int rad = SCX(r);

	fprintf(TEXFILE, "    \\special{ar %d %d %d %d 0.0 6.2832}%%\n",
		TRX(xc), TRY(yc), rad, rad);
}
