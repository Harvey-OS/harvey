#include "art.h"
/*
 * return the closest point on line ip to point testp
 */
Dpoint nearline(Item *ip, Dpoint testp){
	return nearseg(ip->p[0], ip->p[1], testp);
}
void drawline(Item *ip, int bits, Bitmap *b, Fcode c){
	segment(b, D2P(ip->p[0]), D2P(ip->p[1]), bits, c);
}
void editline(void){
	Flt dm=dist(arg[0], dmidpt(selection->p[0], selection->p[1]));
	if(dist(arg[0], selection->p[0])<dm){
		hotpoint(selection->p[1]);
		track(movep, 0, selection);
	}
	else if(dist(arg[0], selection->p[1])<dm){
		hotpoint(selection->p[0]);
		track(movep, 1, selection);
	}
	else
		track(moveall, 0, selection);
}
void translateline(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
	ip->p[1]=dadd(ip->p[1], delta);
}
void deleteline(Item *ip){
}
void writeline(Item *ip, int f){
	fprint(f, "l %.3f %.3f %.3f %.3f\n", ip->p[0].x, ip->p[0].y, ip->p[1].x, ip->p[1].y);
}
void activateline(Item *ip){
	hotline(ip->p[0], ip->p[1], L|R);
}
/*
 * Clipping is undoubtedly quicker than this nonsense
 */
int inboxline(Item *ip, Drectangle r){
	Dpoint i;
	return dptinrect(ip->p[0], r)
	    || seginterseg(r.min, Dpt(r.min.x, r.max.y), ip->p[0], ip->p[1], &i)
	    || seginterseg(r.min, Dpt(r.max.x, r.min.y), ip->p[0], ip->p[1], &i)
	    || seginterseg(r.max, Dpt(r.min.x, r.max.y), ip->p[0], ip->p[1], &i)
	    || seginterseg(r.max, Dpt(r.max.x, r.min.y), ip->p[0], ip->p[1], &i);
}
Drectangle bboxline(Item *ip){
	return drcanon(Drpt(ip->p[0], ip->p[1]));
}
Dpoint nearvertline(Item *ip, Dpoint testp){
	Dpoint mid=dmidpt(ip->p[0], ip->p[1]);
	Flt dm=dist(testp, mid);
	if(dist(ip->p[0], testp)<dm) return ip->p[0];
	if(dist(ip->p[1], testp)<dm) return ip->p[1];
	return mid;
}
Itemfns linefns={
	deleteline,
	writeline,
	activateline,
	nearline,
	drawline,
	editline,
	translateline,
	inboxline,
	bboxline,
	nearvertline,
};
Item *addline(Item *head, Dpoint p0, Dpoint p1){
	return additem(head, LINE, 0., 0, 0, 0, &linefns, 2, p0, p1);
}
