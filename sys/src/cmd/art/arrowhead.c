#include "art.h"
#define	ARROWHT		.1
#define	ARROWWID	.05
void arrowhead(Dpoint p, Dpoint q, int bits, Bitmap *b, Fcode c){
	double d=dist(p, q);
	Dpoint dir;
	if(d<.01) return;
	dir.x=(q.x-p.x)/d;
	dir.y=(q.y-p.y)/d;
	q.x=p.x+dir.x*ARROWHT+dir.y*.5*ARROWWID;
	q.y=p.y+dir.y*ARROWHT-dir.x*.5*ARROWWID;
	segment(b, D2P(p), D2P(q), bits, c);
	q.x=p.x+dir.x*ARROWHT-dir.y*.5*ARROWWID;
	q.y=p.y+dir.y*ARROWHT+dir.x*.5*ARROWWID;
	segment(b, D2P(p), D2P(q), bits, c);
}
void writestyle(int f, int style){
	if(style){
		fprint(f, " S ");
		if(style&ARROW0) fprint(f, "<");
		if(style&ARROW1) fprint(f, ">");
		if(style&DASH) fprint(f, "-");
		if(style&DOT) fprint(f, ".");
	}
}
