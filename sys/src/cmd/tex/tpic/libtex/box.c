#include "tex.h"
box(x0, y0, x1, y1) 
double	x0, y0, x1, y1;
{
	fprintf(TEXFILE,"    \\special{pa %d %d}",TRX(x0),TRY(y0));
	fprintf(TEXFILE,"\\special{pa %d %d}",TRX(x1),TRY(y0));
	fprintf(TEXFILE,"\\special{pa %d %d}",TRX(x1),TRY(y1));
	fprintf(TEXFILE,"\\special{pa %d %d}",TRX(x0),TRY(y1));
	fprintf(TEXFILE,"\\special{pa %d %d}",TRX(x0),TRY(y0));
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
