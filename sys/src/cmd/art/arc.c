#include "art.h"
/*
 * return the closest point on arc ip to point testp.
 */
Dpoint neararc(Item *ip, Dpoint testp){
	Flt d;
	Dpoint p, cen;
	cen=circumcenter(ip->p[0], ip->p[1], ip->p[2]);
	p=dsub(testp, cen);
	d=dlen(p);
	if(d!=0){
		p=dlerp(cen, p, dist(ip->p[0], cen)/d);
		if(triarea(ip->p[0], p, ip->p[2])*triarea(ip->p[0], ip->p[1], ip->p[2])>0) return p;
	}
	if(dist(testp, ip->p[0])<dist(testp, ip->p[2])) return ip->p[0];
	return ip->p[2];
}
void drawarc(Item *ip, int bits, Bitmap *b, Fcode c){
	if(pldist(ip->p[1], ip->p[0], ip->p[2])<CLOSE)	/* not quite right, but ok */
		segment(b, D2P(ip->p[0]), D2P(ip->p[2]), bits, c);
	else if(triarea(ip->p[0], ip->p[1], ip->p[2])>0.)
		arc(b, D2P(circumcenter(ip->p[0], ip->p[1], ip->p[2])),
			D2P(ip->p[0]), D2P(ip->p[2]), bits, c);
	else
		arc(b, D2P(circumcenter(ip->p[0], ip->p[1], ip->p[2])),
			D2P(ip->p[2]), D2P(ip->p[0]), bits, c);
}
void editarc(void){
	Flt l0=dist(arg[0], selection->p[0]);
	Flt l1=dist(arg[0], selection->p[1]);
	Flt l2=dist(arg[0], selection->p[2]);
	if(l0<l1 && l0<l2){
		hotpoint(selection->p[1]);
		hotpoint(selection->p[2]);
		track(movep, 0, selection);
	}
	else if(l1<l2){
		hotpoint(selection->p[0]);
		hotpoint(selection->p[2]);
		track(movep, 1, selection);
	}
	else{
		hotpoint(selection->p[0]);
		hotpoint(selection->p[1]);
		track(movep, 2, selection);
	}
}
void translatearc(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
	ip->p[1]=dadd(ip->p[1], delta);
	ip->p[2]=dadd(ip->p[2], delta);
}
void deletearc(Item *ip){
}
void writearc(Item *ip, int f){
	fprint(f, "a %.3f %.3f %.3f %.3f %.3f %.3f\n",
		ip->p[0].x, ip->p[0].y, ip->p[1].x, ip->p[1].y, ip->p[2].x, ip->p[2].y);
}
void activatearc(Item *ip){
	hotarc(ip->p[0], ip->p[1], ip->p[2]);
}
int inboxarc(Item *ip, Drectangle r){
	Dpoint i[2];
	return dptinrect(ip->p[0], r)
	    || seginterarc(r.min, Dpt(r.min.x, r.max.y), ip->p[0], ip->p[1], ip->p[2], i)
	    || seginterarc(r.min, Dpt(r.max.x, r.min.y), ip->p[0], ip->p[1], ip->p[2], i)
	    || seginterarc(r.max, Dpt(r.min.x, r.max.y), ip->p[0], ip->p[1], ip->p[2], i)
	    || seginterarc(r.max, Dpt(r.max.x, r.min.y), ip->p[0], ip->p[1], ip->p[2], i);
}
/*
 * This can overestimate the size of the box.
 */
Drectangle bboxarc(Item *ip){
	Drectangle r;
	r.min=dsub(ip->p[0], Dpt(ip->r, ip->r));
	r.max=dadd(ip->p[0], Dpt(ip->r, ip->r));
	return r;
}
Dpoint nearvertarc(Item *ip, Dpoint testp){
	if(dist(ip->p[0], testp)<dist(ip->p[2], testp)) return ip->p[0];
	return ip->p[2];
}
Itemfns arcfns={
	deletearc,
	writearc,
	activatearc,
	neararc,
	drawarc,
	editarc,
	translatearc,
	inboxarc,
	bboxarc,
	nearvertarc,
};
Item *addarc(Item *head, Dpoint p0, Dpoint p1, Dpoint p2){
	return additem(head, ARC, 0., 0, 0, 0, &arcfns, 3, p0, p1, p2);
}
