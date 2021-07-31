/*
 * miscellaneous geometric functions
 * dadd(p,q)		a+b
 * dsub(p,q)		a-b
 * dlerp(p,d,a)		p+a*d
 * dlen(p)		sqrt(p.x^2 + p.y^2)
 * datan2(x,y)		atan2(x, y) in degrees
 * same(x,y)		fabs(x-y)<(.1 pixel)
 * deqpt(p,q)		same(p.x, q.x) && same(p.y, q.y)
 * D2P(p)		convert Dpoint (inch coords, y up) to Point (pixel coords, y down),
 *			mapping origin to lower left corner of screen window
 * P2D(p)		convert Point to Dpoint, inverse of D2P
 * dmul(p, a)		p*a
 * dreflect(p,q,r)	reflect p through line [q,r]
 * dmidpt(p,q)		(p+q)/2
 * circumcenter(p,q,r)	circumcenter of triangle pqr
 * triarea(p,q,r)	area of triangle pqr
 * circintercirc(c0,r0,c1,r1,i0,i1)
 *			store intersection points of circles (c0,r0) and (c1,r1) in *i0
 *			and *i1.  return the number of intersections.
 * segintercircle(p0,p1,cen,r,i0,i1)
 *			store intersections of line segment [p0,p1] and circle (cen,r) in
 *			*i0 and *i1.  return the number of intersections.
 * seginterseg(p0,p1,q0,q1,i)
 *			store intersection of segment [p0,p1] with [q0,q1] in *i.  return
 *			number of intersections  (0 or 1).
 * seginterarc(p0,p1,a0,a1,a2,i0,i1)
 *			store intersections of line segment [p0,p1] and arc (a0,a1,a2) in
 *			*i0 and *i1.  return the number of intersections.
 * dptinrect(p,r)	is point p in rectangle r?
 * drectxrect(r1,r2)	do rectangles r1 and r2 intersect?
 * drcanon(r)		make rectangle canonical (min.x<=max.x && min.y<=max.y)
 * pldist(p,q,r)	distance of point p from segment [q,r]
 * dunion(r, s)		smallest rectangle containing two rectangles
 */
#include "art.h"
Dpoint dadd(Dpoint p, Dpoint q){
	p.x+=q.x;
	p.y+=q.y;
	return p;
}
Dpoint dsub(Dpoint p, Dpoint q){
	p.x-=q.x;
	p.y-=q.y;
	return p;
}
Dpoint dlerp(Dpoint p, Dpoint dp, Flt alpha){
	p.x+=dp.x*alpha;
	p.y+=dp.y*alpha;
	return p;
}
Dpoint dcomb2(Dpoint p, Flt wp, Dpoint q, Flt wq){
	Flt den=wp+wq;
	p.x=(p.x*wp+q.x*wq)/den;
	p.y=(p.y*wp+q.y*wq)/den;
	return p;
}
Dpoint dcomb3(Dpoint p, Flt wp, Dpoint q, Flt wq, Dpoint r, Flt wr){
	Flt den=wp+wq+wr;
	p.x=(p.x*wp+q.x*wq+r.x*wr)/den;
	p.y=(p.y*wp+q.y*wq+r.y*wr)/den;
	return p;
}
Flt dlen(Dpoint p){
	return sqrt(p.x*p.x+p.y*p.y);
}
/*
 * arctangent in degrees
 */
Flt datan2(Flt x, Flt y){
	return degrees(atan2(x, y));
}
/*
 * the angle subtended by pq at the origin
 */
Flt angle(Dpoint p, Dpoint q){
	return datan2(p.x*q.y+p.y*q.x, p.x*q.x-p.y*q.y);
}
/*
 * absolute equality comparison
 */
int same(Flt a, Flt b){
	return fabs(a-b)<CLOSE;
}
int deqpt(Dpoint p, Dpoint q){
	return same(p.x, q.x) && same(p.y, q.y);
}
Point D2P(Dpoint d){
	Point p;
	p.x=(int)floor(d.x*DPI+.5)+dwgbox.min.x;
	p.y=dwgbox.max.y-1-(int)floor(d.y*DPI+.5);
	return p;
}
Dpoint P2D(Point p){
	Dpoint d;
	d.x=(p.x-dwgbox.min.x)/DPI;
	d.y=(dwgbox.max.y-1-p.y)/DPI;
	return d;
}
Dpoint dmul(Dpoint p, Flt m){
	p.x*=m;
	p.y*=m;
	return p;
}
Dpoint dreflect(Dpoint p, Dpoint p0, Dpoint p1){
	Dpoint a, b;
	a=dsub(p, p0);
	b=dsub(p1, p0);
	return dlerp(dsub(p0, a), b, 2*(a.x*b.x+a.y*b.y)/(b.x*b.x+b.y*b.y));
}
Dpoint dmidpt(Dpoint p, Dpoint q){
	p.x=(p.x+q.x)/2;
	p.y=(p.y+q.y)/2;
	return p;
}
/*
 * Return the circumcenter of triangle pqr
 */
Dpoint circumcenter(Dpoint p, Dpoint q, Dpoint r){
	Dpoint p0, d0, p1, d1, p10;
	Flt den;
	p0=dmidpt(p, q);
	p1=dmidpt(p, r);
	d0.x=q.y-p.y; d0.y=p.x-q.x;
	d1.x=r.y-p.y; d1.y=p.x-r.x;
	den=d1.x*d0.y-d0.x*d1.y;
	if(fabs(den)<SMALL) return p0;			/* parallel, punt */
	p10=dsub(p1, p0);
	return dlerp(p0, d0, (p10.y*d1.x-p10.x*d1.y)/den);
}
Flt triarea(Dpoint p0, Dpoint p1, Dpoint p2){
	return (p1.x-p0.x)*(p1.y+p0.y)+(p2.x-p1.x)*(p2.y+p1.y)+(p0.x-p2.x)*(p0.y+p2.y);
}
/*
 * Return the nearest point on segment p0:p1 to the test point
 */
Dpoint nearseg(Dpoint p0, Dpoint p1, Dpoint testp){
	Flt num, den;
	Dpoint q, r;
	q=dsub(p1, p0);
	r=dsub(testp, p0);
	num=q.x*r.x+q.y*r.y;
	if(num<=0) return p0;
	den=q.x*q.x+q.y*q.y;
	if(num>=den) return p1;
	return dlerp(p0, q, num/den);
}
/*
 * Compute intersections of circles centered at c[01] with
 * radii r[01].  Return number of intersections, store values
 * through i[01]
 */
int circintercirc(Dpoint c0, Flt r0, Dpoint c1, Flt r1, Dpoint *i){
	Flt r0sq=r0*r0, r1sq=r1*r1;
	Dpoint c10=dsub(c1, c0), p, d;
	Flt dsq=c10.x*c10.x+c10.y*c10.y;
	Flt rdiff, rsum, root, dinv, alpha;
	if(dsq<=SMALL*SMALL) return 0;	/* common origin */
	rdiff=r1sq-r0sq;
	rsum=r0sq+r1sq;
	root=2.*rsum*dsq-dsq*dsq-rdiff*rdiff;
	if(root<-SMALL) return 0;	/* circles don't touch */
	dinv=.5/dsq;
	alpha=.5-rdiff*dinv;
	p=dlerp(c0, c10, alpha);
	if(root<SMALL){			/* circles just touch */
		i[0]=p;
		return 1;
	}
	root=dinv*sqrt(root);
	d.x=-c10.y*root;
	d.y=c10.x*root;
	i[0]=dadd(p, d);
	i[1]=dsub(p, d);
	return 2;
}
/*
 * Compute intersections of a segment (p0, p1) and circle (cen, r).
 * Return number of intersections, store values through i[01]
 */
int segintercircle(Dpoint p0, Dpoint p1, Dpoint cen, Flt r, Dpoint *i){
	Dpoint d0, pc;
	Flt a, b, det, num;
	int ninter;
	d0=dsub(p1, p0);		/* p=p0 + d0*alpha */
	pc=dsub(p0, cen);
	/*
	 * a*alpha*alpha + 2*b*alpha + c == 0, where
	 * a=dot(d0, d0)
	 * b=dot(d0, pc)
	 * c=dot(pc, pc) - r*r
	 */
	a=d0.x*d0.x+d0.y*d0.y;		/* cannot be negative */
	if(a<SMALL) return 0;		/* degenerate segment: shouldn't happen */
	b=d0.x*pc.x+d0.y*pc.y;
	det=b*b-a*(pc.x*pc.x+pc.y*pc.y-r*r);
	if(det<0) return 0;		/* miss circle? */
	det=sqrt(det);
	num=-b-det;
	ninter=0;
	if(0<=num && num<=a){
		*i++=dlerp(p0, d0, num/a);
		ninter++;
	}
	num=-b+det;
	if(0<=num && num<=a){
		*i=dlerp(p0, d0, num/a);
		ninter++;
	}
	return ninter;
}
/*
 * Store intersection of segments (p0, p1), (q0, q1) in *i.
 * Return number of intersections (0 or 1).
 */
int seginterseg(Dpoint p0, Dpoint p1, Dpoint q0, Dpoint q1, Dpoint *i){
	Flt den, alpha, beta;
	Dpoint dp, dq, pq;
	dp=dsub(p1, p0);			/* p=p0+dp*alpha/den */
	dq=dsub(q1, q0);			/* p=q0+dq*beta/den */
	den=dq.x*dp.y-dp.x*dq.y;
	if(den==0) return 0;			/* parallel */
	pq=dsub(q0, p0);
	alpha=pq.y*dq.x-pq.x*dq.y;
	beta=pq.y*dp.x-pq.x*dp.y;
	if(den<0){
		den=-den;
		alpha=-alpha;
		beta=-beta;
	}
	if(alpha<0 || den<alpha			/* off the end of i0, or */
	|| beta<0 || den<beta) return 0;	/* off the end of i1 */
	*i=dlerp(p0, dp, alpha/den);
	return 1;
}
Flt dist(Dpoint p, Dpoint q){
	p.x-=q.x;
	p.y-=q.y;
	return sqrt(p.x*p.x+p.y*p.y);
}
/*
 * Compute intersections of a segment (p0, p1) and arc (a0, a1, a2).
 * Return number of intersections, store values through i[01]
 */
int seginterarc(Dpoint p0, Dpoint p1, Dpoint a0, Dpoint a1, Dpoint a2, Dpoint *i){
	Dpoint cen, ia[2];
	Flt r, area;
	int ninter=0, n, j;
	if(pldist(a0, a1, a2)<CLOSE) return seginterseg(p0, p1, a0, a2, i);
	cen=circumcenter(a0, a1, a2);
	r=dist(cen, a0);
	n=segintercircle(p0, p1, cen, r, ia);
	if(n==0) return 0;
	area=triarea(a0, a1, a2);
	for(j=0;j!=n;j++)
		if(area*triarea(a0, ia[j], a2)>=0.){
			*i++=ia[j];
			ninter++;
		}
	return ninter;
}
int circinterarc(Dpoint cen, Flt r, Dpoint a0, Dpoint a1, Dpoint a2, Dpoint *i){
	Dpoint acen, ia[2];
	Flt ar, area;
	int ninter=0, n, j;
	if(pldist(a0, a1, a2)<CLOSE) return segintercircle(a0, a2, cen, r, i);
	acen=circumcenter(a0, a1, a2);
	ar=dist(acen, a0);
	n=circintercirc(acen, ar, cen, r, ia);
	if(n==0) return 0;
	area=triarea(a0, a1, a2);
	for(j=0;j!=n;j++)
		if(area*triarea(a0, ia[j], a2)>=0.){
			*i++=ia[j];
			ninter++;
		}
	return ninter;
}
int arcinterarc(Dpoint a0, Dpoint a1, Dpoint a2, Dpoint b0, Dpoint b1, Dpoint b2, Dpoint *i){
	Dpoint acen, ia[2];
	Flt ar, area;
	int ninter=0, n, j;
	if(pldist(a0, a1, a2)<CLOSE) return seginterarc(a0, a2, b0, b1, b2, i);
	acen=circumcenter(a0, a1, a2);
	ar=dist(acen, a0);
	n=circinterarc(acen, ar, b0, b1, b2, ia);
	if(n==0) return 0;
	area=triarea(a0, a1, a2);
	for(j=0;j!=n;j++)
		if(area*triarea(a0, ia[j], a2)>=0.){
			*i++=ia[j];
			ninter++;
		}
	return ninter;
}	
int dptinrect(Dpoint p, Drectangle r){
	return r.min.x<=p.x && p.x<=r.max.x && r.min.y<=p.y && p.y<=r.max.y;
}
int drectxrect(Drectangle r1, Drectangle r2){
	return r1.min.x<=r2.max.x && r2.min.x<=r1.max.x
	    && r1.min.y<=r2.max.y && r2.min.y<=r1.max.y;
}
Drectangle drcanon(Drectangle r){
	Flt t;
	if(r.min.x>r.max.x){ t=r.min.x; r.min.x=r.max.x; r.max.x=t; }
	if(r.min.y>r.max.y){ t=r.min.y; r.min.y=r.max.y; r.max.y=t; }
	return r;
}
/*
 * distance from point p to line [p0,p1]
 */
Flt pldist(Dpoint p, Dpoint p0, Dpoint p1){
	Dpoint d, e;
	Flt dd, de, dsq;
	d=dsub(p1, p0);
	e=dsub(p, p0);
	dd=d.x*d.x+d.y*d.y;
	de=d.x*e.x+d.y*e.y;
	if(dd<SMALL*SMALL) return sqrt(e.x*e.x+e.y*e.y);
	dsq=e.x*e.x+e.y*e.y-de*de/dd;
	if(dsq<SMALL*SMALL) return 0;
	return sqrt(dsq);
}
Drectangle dunion(Drectangle r, Drectangle s){
	if(s.min.x<r.min.x) r.min.x=s.min.x;
	if(s.min.y<r.min.y) r.min.y=s.min.y;
	if(s.max.x>r.max.x) r.max.x=s.max.x;
	if(s.max.y>r.max.y) r.max.y=s.max.y;
	return r;
}
Drectangle draddp(Drectangle r, Dpoint p){
	return Drpt(dadd(r.min, p), dadd(r.max, p));
}
