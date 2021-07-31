#include "mplot.h"
void disk(double xc, double yc, double r){
	Point p;
	int rad;
	p.x=SCX(xc);
	p.y=SCY(yc);
	if (r < 0) 
		rad=SCR(-r);
	else
		rad=SCR(r);
	disc(&screen, p, rad, e1->foregr, S);
}
