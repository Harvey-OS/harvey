#include "tex.h"

vec(xx, yy) 
double	xx, yy; 
{
	fprintf(TEXFILE,"    \\special{pa %d %d}",TRX(e1->copyx),TRY(e1->copyy));
	e1->copyx = xx; 
	e1->copyy = yy;
	fprintf(TEXFILE,"\\special{pa %d %d}",TRX(xx),TRY(yy));
	switch(e1->pen){
	case DASHPEN:
		fprintf(TEXFILE,"\\special{da %6.3f}%%\n", e1->dashlen); break;
	case DOTPEN:
		fprintf(TEXFILE,"\\special{dt %6.3f}%%\n", e1->dashlen); break;
	case SOLIDPEN:
	default:
		fprintf(TEXFILE,"\\special{fp}%%\n"); break;
	}
}
