#include "mplot.h"
Bitmap *offscreen=&screen;
/*
 * Clear the window from x0, y0 to x1, y1 (inclusive) to color c
 */
void m_clrwin(int x0, int y0, int x1, int y1, int c){
	int y, hgt;
	x1++;
	y1++;
	if(c<=0)
		bitblt(offscreen, Pt(x0, y0), offscreen, Rect(x0, y0, x1, y1), Zero);
	else if(c>=(2<<screen.ldepth)-1)
		bitblt(offscreen, Pt(x0, y0), offscreen, Rect(x0, y0, x1, y1), F);
	else{
		segment(offscreen, Pt(x0, y0), Pt(x1, y0), c, S);
		for(y=y0+1,hgt=1;y<y1;y+=hgt,hgt*=2){
			if(y+hgt>y1) hgt=y1-y;
			bitblt(offscreen, Pt(x0, y), offscreen, Rect(x0, y0, x1, y0+hgt), S);
		}
	}
}
/*
 * Draw text between pointers p and q with first character centered at x, y.
 * Use color c.  Centered if cen is non-zero, right-justified if right is non-zero.
 * Returns the y coordinate for any following line of text.
 * Bug: color is ignored.
 */
int m_text(int x, int y, char *p, char *q, int c, int cen, int right){
	Point tsize;
	USED(c);
	*q='\0';
	tsize=strsize(font, p);
	if(cen) x -= tsize.x/2;
	else if(right) x -= tsize.x;
	string(offscreen, Pt(x, y-tsize.y/2), font, p, S|D);
	return y+tsize.y;
}
/*
 * Draw the vector from x0, y0 to x1, y1 in color c.
 * Clipped by caller
 */
void m_vector(int x0, int y0, int x1, int y1, int c){
	if(c<0) c=0;
	if(c>(1<<(1<<screen.ldepth))-1) c=(2<<screen.ldepth)-1;
	segment(offscreen, Pt(x0, y0), Pt(x1, y1), c, S);
}
/*
 * Startup initialization
 */
void m_initialize(char *s){
	static int first=1;
	int dx, dy;
	USED(s);
	if(first){
		binit(0,0,0);
		clipminx=mapminx=screen.r.min.x+4;
		clipminy=mapminy=screen.r.min.y+4;
		clipmaxx=mapmaxx=screen.r.max.x-5;
		clipmaxy=mapmaxy=screen.r.max.y-5;
		dx=clipmaxx-clipminx;
		dy=clipmaxy-clipminy;
		if(dx>dy){
			mapminx+=(dx-dy)/2;
			mapmaxx=mapminx+dy;
		}
		else{
			mapminy+=(dy-dx)/2;
			mapmaxy=mapminy+dx;
		}
		first=0;
	}
}
/*
 * Clean up when finished
 */
void m_finish(void){
	m_swapbuf();
}
void m_swapbuf(void){
	if(offscreen!=&screen)
		bitblt(&screen, offscreen->r.min, offscreen, offscreen->r, S);
	bflush();
}
void m_dblbuf(void){
	if(offscreen==&screen){
		offscreen=balloc(inset(screen.r, 4), screen.ldepth);
		if(offscreen==0){
			fprintf(stderr, "Can't double buffer\n");
			offscreen=&screen;
		}
	}
}
