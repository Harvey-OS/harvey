#include "mplot.h"
void color(char *s){
	int k=bcolor(s);
	if(k>=0) e1->foregr=k;
}
