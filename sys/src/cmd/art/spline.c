/*
 * Splines are catmull-rom w. duplicated knots at ends.
 * Box testing, intersection and nearness tests are done
 * by converting to Bezier form and subdividing.
 *
 * There should be a way to delete a control point from a spline, perhaps
 * via an extra item in menu 3.
 */
#include "art.h"
#define	LEVEL	4
/*
 * return the closest point on spline ip to point testp
 */
Dpoint nearspline(Item *ip, Dpoint testp){
	int i;
	Bezier b;
	Dpoint close=ip->p[0];
	for(i=0;i!=ip->np-1;i++){
		s2b(ip, i, b);
		close=nearbezier(b, testp, close, LEVEL);
	}
	return close;
}
void drawspline(Item *ip, int bits, Bitmap *b, Fcode c){
	int i;
	Bezier p;
	for(i=0;i!=ip->np-1;i++){
		s2b(ip, i, p);
		drawbezier(p, bits, b, c);
	}
}
void editspline(void){
	Flt dmin=dsq(arg[0], selection->p[0]), d;
	int imin=0;
	int i;
	for(i=1;i!=selection->np;i++){
		d=dsq(arg[0], selection->p[i]);
		if(d<dmin){
			imin=i;
			dmin=d;
		}
	}
	for(i=0;i!=selection->np;i++) if(i!=imin) hotpoint(selection->p[i]);
	track(movespline, imin, selection);
}
void translatespline(Item *ip, Dpoint delta){
	int i;
	for(i=0;i!=ip->np;i++) ip->p[i]=dadd(ip->p[i], delta);
}
void deletespline(Item *ip){
}
void writespline(Item *ip, int f){
	int i;
	fprint(f, "s %d", ip->np);
	for(i=0;i!=ip->np;i++) fprint(f, " %.3f %.3f", ip->p[i].x, ip->p[i].y);
	fprint(f, "\n");
}
void activatespline(Item *ip){
	int i;
	hotline(ip->p[0], ip->p[1], L);
	for(i=1;i<ip->np-1;i++)
		hotline(ip->p[i], dadd(ip->p[i], dsub(ip->p[i+1], ip->p[i-1])), L);
	hotline(ip->p[ip->np-1], ip->p[ip->np-2], L);
}
int inboxspline(Item *ip, Drectangle r){
	int i;
	Bezier b;
	for(i=0;i!=ip->np-1;i++){
		s2b(ip, i, b);
		if(inboxbezier(b, r, LEVEL)) return 1;
	}
	return 0;
}
Drectangle bboxspline(Item *ip){
	int i;
	Bezier b;
	Drectangle r;
	s2b(ip, 0, b);
	r=bboxbezier(b);
	for(i=1;i!=ip->np-1;i++){
		s2b(ip, i, b);
		r=dunion(r, bboxbezier(b));
	}
	return r;
}
Dpoint nearvertspline(Item *ip, Dpoint testp){
	Flt dmin=dsq(testp, ip->p[0]), d;
	Dpoint close=ip->p[0], p;
	int imin=0;
	int i;
	d=dsq(testp, ip->p[ip->np-1]);
	if(d<dmin){
		imin=ip->np-1;
		dmin=d;
		close=ip->p[imin];
	}
	for(i=1;i<ip->np-1;i++){
		p=dcomb3(ip->p[i-1], 1., ip->p[i], 2., ip->p[i+1], 1.);
		d=dsq(testp, p);
		if(d<dmin){
			imin=i;
			dmin=d;
			close=p;
		}
	}
	return close;
}
Itemfns splinefns={
	deletespline,
	writespline,
	activatespline,
	nearspline,
	drawspline,
	editspline,
	translatespline,
	inboxspline,
	bboxspline,
	nearvertspline,
};
Item *addspline(Item *head, Dpoint p0, Dpoint p1){
	return additem(head, SPLINE, 0., 0, 0, 0, &splinefns, 2, p0, p1);
}
int morespline(Item *ip, Dpoint p){
	int i;
	if(ip->type==LINE) ip->type=SPLINE;
	else if(ip->type!=SPLINE) fatal("bad type in morespline");
	ip->np++;
	ip->p=realloc(ip->p, ip->np*sizeof(Dpoint));
	if(dsq(ip->p[0], p)<dsq(ip->p[ip->np-2], p)){
		for(i=ip->np-1;i!=0;--i) ip->p[i]=ip->p[i-1];
		ip->p[0]=p;
		return 0;
	}
	ip->p[ip->np-1]=p;
	return ip->np-1;
}
void s2b(Item *ip, int i0, Bezier b){
	Dpoint *p=&ip->p[i0];
	if(i0<0 || ip->np-1<=i0) fatal("bad i1 in s2b");
	if(i0==0){
		b[0]=p[0];
		b[1]=dcomb2(p[0], 2., p[1], 1.);
	}
	else{
		b[0]=dcomb3(p[-1], 1., p[0], 2., p[1], 1.);
		b[1]=dcomb3(p[-1], 1., p[0], 6., p[1], 5.);
	}
	if(i0==ip->np-2){
		b[2]=dcomb2(p[0], 1., p[1], 2.);
		b[3]=p[1];
	}
	else{
		b[2]=dcomb3(p[0], 5., p[1], 6., p[2], 1.);
		b[3]=dcomb3(p[0], 1., p[1], 2., p[2], 1.);
	}
}
int seginterspline(Dpoint p0, Dpoint p1, Item *s, Dpoint *i){
	int j, n;
	Bezier b;
	n=0;
	for(j=0;j!=s->np-1;j++){
		s2b(s, j, b);
		n+=seginterbezier(p0, p1, b, i+n, LEVEL);
	}
	return n;
}
int splineinterspline(Item *s1, Item *s2, Dpoint *i){
	int j, k, n;
	Bezier b1, b2;
	n=0;
	for(j=0;j!=s1->np-1;j++){
		s2b(s1, j, b1);
		for(k=0;k!=s2->np-1;k++) if(s1!=s2 || k<j){
			s2b(s2, k, b2);
			n+=bezierinterbezier(b1, b2, i+n, LEVEL);
		}
	}
	return n;
}
void movespline(Item *ip, int i, Dpoint p){
	drawpart(ip, i, scoffs);
	if(i==0 || i==ip->np-1) ip->p[i]=p;
	else ip->p[i]=dsub(dmul(p, 2.), dmul(dadd(ip->p[i-1], ip->p[i+1]), .5));
	drawpart(ip, i, scoffs);
}
void drawpart(Item *ip, int index, Dpoint offs){
	Bezier p;
	int i, i0=index-2, i1=index+2, j;
	if(i0<0) i0=0;
	if(i1>=ip->np) i1=ip->np-1;
	for(i=i0;i!=i1;i++){
		s2b(ip, i, p);
		for(j=0;j!=ORDER;j++) p[j]=dadd(p[j], offs);
		drawbezier(p, DARK, &screen, S^D);
	}
}
