#include "mplot.h"
void dpoint(double x, double y){
	point(&screen, Pt(SCX(x), SCY(y)), e1->foregr, S);
	move(x, y);
}
