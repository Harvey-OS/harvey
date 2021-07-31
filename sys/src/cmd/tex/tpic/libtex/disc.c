#include "tex.h"
disc(xc, yc, r)
double xc, yc, r;
{
	fprintf(TEXFILE, "    \\special{bk}%%\n");
	circle(xc, yc, r);
}
