/*
 * Routines to mangle Bezier curves (of some fixed order)
 *	bsplit(b, l, r)			split a bezier curve at its midpoint
 *	Rectangle bboxbezier(b)		bounding box of a bezier curve
 *	Dpoint nearbezier(b, p, LEVEL)	return the nearest point to the given bezier curve
 *	inboxbezier(b, box, LEVEL)	does the bezier curve intersect the box?
 *	drawbezier(b, bits, bitmap, c)	draw a bezier curve in a bitmap
 */
#include "art.h"
Flt min(Flt a, Flt b){ return a<b?a:b; }
Flt max(Flt a, Flt b){ return a>b?a:b; }
Flt dsq(Dpoint p, Dpoint q){
	p.x-=q.x;
	p.y-=q.y;
	return p.x*p.x+p.y*p.y;
}
void bsplit(Bezier b, Bezier l, Bezier r){
	Dpoint subd[ORDER][ORDER];
	int i, j;
	Dpoint *p, *q, *ep;
	ep=&subd[0][ORDER];
	for(p=&subd[0][0],q=b;p!=ep;p++, q++) *p=*q;
	for(i=1;i!=ORDER;i++){
		ep=&subd[i][ORDER-i];
		for(p=&subd[i][0],q=&subd[i-1][0];p!=ep;p++, q++)
			*p=dmidpt(q[0], q[1]);
	}
	ep=&l[ORDER];
	for(p=l,q=&subd[0][0];p!=ep;p++,q+=ORDER) *p=*q;
	ep=&r[ORDER];
	for(p=r,q=&subd[ORDER-1][0];p!=ep;p++,q+=1-ORDER) *p=*q;
}
Dpoint closevert(Bezier b, Dpoint testp){
	Flt d, mind;
	Dpoint close, p, *bp, *ebp=b+ORDER-1;
	close=b[0];
	mind=dsq(close, testp);
	for(bp=b;bp!=ebp;bp++){
		p=nearseg(bp[0], bp[1], testp);
		d=dsq(p, testp);
		if(d<mind){
			mind=d;
			close=p;
		}
	}
	return close;
}
Drectangle bboxbezier(Bezier b){
	int i;
	Drectangle box;
	Dpoint *bp, *ebp=b+ORDER;
	box.min=b[0];
	box.max=b[0];
	for(bp=b+1;bp!=ebp;bp++){
		if(bp->x<box.min.x) box.min.x=bp->x;
		if(bp->x>box.max.x) box.max.x=bp->x;
		if(bp->y<box.min.y) box.min.y=bp->y;
		if(bp->y>box.max.y) box.max.y=bp->y;
	}
	return box;
}
/*
 * If close is closer to p than any point on b, return close.
 * Otherwise, return the closest point to p on b.
 */
Dpoint nearbezier(Bezier b, Dpoint p, Dpoint close, int level){
	Drectangle bbox;
	Bezier l, r;
	Dpoint d;
	bbox=bboxbezier(b);
	d.x=p.x<bbox.min.x?bbox.min.x:p.x<=bbox.max.x?p.x:bbox.max.x;
	d.y=p.y<bbox.min.y?bbox.min.y:p.y<=bbox.max.y?p.y:bbox.max.y;
	if(dsq(p, d)>dsq(p, close)) return close;
	if(level==0) return closevert(b, p);
	bsplit(b, l, r);
	return nearbezier(r, p, nearbezier(l, p, close, level-1), level-1);
}
int inboxbezier(Bezier b, Drectangle box, int level){
	Drectangle bound;
	Bezier l, r;
	bound=bboxbezier(b);
	if(bound.max.x<box.min.x || box.max.x<bound.min.x
	|| bound.max.y<box.min.y || box.max.y<bound.min.y) return 0;
	if(box.min.x<=bound.min.x && bound.max.x<=box.max.x
	&& box.min.y<=bound.min.y && bound.max.y<=box.max.y) return 1;
	if(level==0) return 1;
	bsplit(b, l, r);
	return inboxbezier(l, box, level-1) || inboxbezier(r, box, level-1);
}
int seginterbezier(Dpoint p0, Dpoint p1, Bezier b, Dpoint *i, int level){
	Drectangle box;
	Bezier l, r;
	int n;
	if(level==0) return seginterseg(p0, p1, b[0], b[3], i);
	box=bboxbezier(b);
	if(p0.x<box.min.x && p1.x<box.min.x
	|| p0.x>box.max.x && p1.x>box.max.x
	|| p0.y<box.min.y && p1.y<box.min.y
	|| p0.y>box.max.y && p1.y>box.max.y) return 0;
	bsplit(b, l, r);
	n=seginterbezier(p0, p1, l, i, level-1);
	return n+seginterbezier(p0, p1, r, i+n, level-1);
}
int bezierinterbezier(Bezier b1, Bezier b2, Dpoint *i, int level){
	Drectangle box1, box2;
	Bezier l1, r1, l2, r2;
	int n;
	if(level==0) return seginterseg(b1[0], b1[3], b2[0], b2[3], i);
	box1=bboxbezier(b1);
	box2=bboxbezier(b2);
	if(!drectxrect(box1, box2)) return 0;
	bsplit(b1, l1, r1);
	bsplit(b2, l2, r2);
	n =bezierinterbezier(l1, l2, i, level-1);
	n+=bezierinterbezier(l1, r2, i+n, level-1);
	n+=bezierinterbezier(r1, r2, i+n, level-1);
	return n+bezierinterbezier(r1, l2, i+n, level-1);
}
int bstraight(Bezier b){
	int i;
	for(i=1;i!=ORDER-1;i++) if(pldist(b[i], b[0], b[ORDER-1])>.5/DPI) return 0;
	return 1;
}
void drawbezier(Bezier p, int bits, Bitmap *b, Fcode c){
	Bezier l, r;
	if(bstraight(p)){
		segment(b, D2P(p[0]), D2P(p[ORDER-1]), bits, c);
		return;
	}
	bsplit(p, l, r);
	drawbezier(l, bits, b, c);
	drawbezier(r, bits, b, c);
}
