/*
 * Code to deal with the grid
 */
#include "art.h"
Grid grid[NGRID]={
	0., 0., .1, .1, " 0,0+.1,.1"
};
char *mgrid[NGRID+4]={ "grid", " measure", " off", grid[0].button, 0};
int ngrid=1;
int gridsel=-1;
void Mgrid(void){
	Dpoint delta;
	if(narg<2){
		msg("first pick two points, then measure grid");
		return;
	}
	delta=dsub(arg[0], arg[1]);
	if(delta.x<0) delta.x=-delta.x;
	if(delta.y<0) delta.y=-delta.y;
	if(delta.x==0 || delta.y==0)
		msg("select the diagonal of the grid cell");
	else{
		msg("grid %.2f,%.2f+%.2f,%.2f", arg[0].x, arg[0].y, delta.x, delta.y);
		if(ngrid!=NGRID){
			grid[ngrid].origin=arg[0];
			grid[ngrid].delta=delta;
			sprint(grid[ngrid].button, " %.2f,%.2f+%.2f,%.2f",
				arg[0].x, arg[0].y, delta.x, delta.y);
			mgrid[ngrid+3]=grid[ngrid].button;
			ngrid++;
			newgrid(ngrid-1);
		}
	}
}
void Ogrid(int n){
	if(n==0) Mgrid();
	else newgrid(n-2);
}
/*
 * return the closest point to testp on the grid.
 * If the grid is off, return a point that's too far away to gravitate to.
 */
Dpoint neargrid(Dpoint testp){
	Dpoint s, o, d;
	if(gridsel==-1) return dadd(testp, Dpt(2.*gravity+1., 0.));
	o=grid[gridsel].origin;
	d=grid[gridsel].delta;
	s.x=d.x==0.?testp.x:o.x+floor((testp.x-o.x)/d.x+.5)*d.x;
	s.y=d.y==0.?testp.y:o.y+floor((testp.y-o.y)/d.y+.5)*d.y;
	return s;
}
void newgrid(int n){
	if(gridsel!=n){
		gridsel=n;
		redraw();
	}
}
void drawgrid(void){
	Dpoint lo, hi, p, d;
	if(gridsel==-1) return;
	if(grid[gridsel].button[0]=='*') grid[gridsel].button[0]=' ';
	else grid[gridsel].button[0]='*';
	lo=neargrid(Dpt(0., 0.));
	hi=neargrid(P2D(Pt(dwgbox.max.x, dwgbox.min.y)));
	d=grid[gridsel].delta;
	for(p.y=lo.y;p.y<=hi.y;p.y+=d.y)
		for(p.x=lo.x;p.x<=hi.x;p.x+=d.x)
			point(&screen, D2P(p), faint, S|D);
}
