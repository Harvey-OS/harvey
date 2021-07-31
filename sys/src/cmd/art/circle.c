#include "art.h"
/*
 * return the closest point on circle ip to point testp.  If testp==ip->p[0], no such point
 * exists, so we return testp.
 */
Dpoint nearcircle(Item *ip, Dpoint testp){
	Flt d;
	Dpoint p;
	p=dsub(testp, ip->p[0]);
	d=dlen(p);
	if(d==0) return testp;
	return dlerp(ip->p[0], p, ip->r/d);
}
void drawcircle(Item *ip, int bits, Bitmap *b, Fcode c){
	circle(b, D2P(ip->p[0]), (int)(ip->r*DPI), bits, c);
}
void editcircle(void){
	if(dist(arg[0], selection->p[0])<.5*selection->r)
		track(movep, 0, selection);
	else{
		hotpoint(selection->p[0]);
		track(mover, 0, selection);
	}
}
void translatecircle(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
}
void deletecircle(Item *ip){
}
void writecircle(Item *ip, int f){
	fprint(f, "c %.3f %.3f %.3f\n", ip->p[0].x, ip->p[0].y, ip->r);
}
void activatecircle(Item *ip){
	hotpoint(ip->p[0]);
}
int inboxcircle(Item *ip, Drectangle r){
	Dpoint i[2];
	return dptinrect(dadd(ip->p[0], Dpt(ip->r, 0.)), r)
	    || segintercircle(r.min, Dpt(r.min.x, r.max.y), ip->p[0], ip->r, i)
	    || segintercircle(r.min, Dpt(r.max.x, r.min.y), ip->p[0], ip->r, i)
	    || segintercircle(r.max, Dpt(r.min.x, r.max.y), ip->p[0], ip->r, i)
	    || segintercircle(r.max, Dpt(r.max.x, r.min.y), ip->p[0], ip->r, i);
}
Drectangle bboxcircle(Item *ip){
	Drectangle r;
	r.min=dsub(ip->p[0], Dpt(ip->r, ip->r));
	r.max=dadd(ip->p[0], Dpt(ip->r, ip->r));
	return r;
}
Dpoint nearvertcircle(Item *ip, Dpoint testp){
	return ip->p[0];
}
Itemfns circlefns={
	deletecircle,
	writecircle,
	activatecircle,
	nearcircle,
	drawcircle,
	editcircle,
	translatecircle,
	inboxcircle,
	bboxcircle,
	nearvertcircle,
};
Item *addcircle(Item *head, Dpoint p0, Flt r){
	return additem(head, CIRCLE, r, 0, 0, 0, &circlefns, 1, p0);
}
