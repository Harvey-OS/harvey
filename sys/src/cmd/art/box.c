#include "art.h"
/*
 * return the closest point on box ip to point testp.
 */
Dpoint nearbox(Item *ip, Dpoint testp){
	Dpoint p=testp, p0=ip->p[0], p1=ip->p[1];
	Flt d0, d1, d2, d3;
	if(p0.x>p1.x){ d0=p0.x; p0.x=p1.x; p1.x=d0; }
	if(p0.y>p1.y){ d0=p0.y; p0.y=p1.y; p1.y=d0; }
	if(p.x<p0.x) p.x=p0.x; else if(p.x>p1.x) p.x=p1.x;
	if(p.y<p0.y) p.y=p0.y; else if(p.y>p1.y) p.y=p1.y;
	d0=p.x-p0.x;
	d1=p1.x-p.x;
	d2=p.y-p0.y;
	d3=p1.y-p.y;
	if(d0<d1 && d0<d2 && d0<d3) return Dpt(p0.x, p.y);
	if(d1<d2 && d1<d3) return Dpt(p1.x, p.y);
	if(d2<d3) return Dpt(p.x, p0.y);
	return Dpt(p.x, p1.y);
}
void drawbox(Item *ip, int bits, Bitmap *b, Fcode c){
	Point p0=D2P(ip->p[0]);
	Point p1=D2P(ip->p[1]);
	segment(b, p0, Pt(p1.x, p0.y), bits, c);
	segment(b, Pt(p1.x, p0.y), p1, bits, c);
	segment(b, p1, Pt(p0.x, p1.y), bits, c);
	segment(b, Pt(p0.x, p1.y), p0, bits, c);
}
#define	right(p) Dpt((p).x+1, (p).y)
#define	up(p)    Dpt((p).x, (p).y+1)
#define	left(p)  Dpt((p).x-1, (p).y)
#define	down(p)  Dpt((p).x, (p).y-1)
void editbox(void){
	Item *s=selection;
	Dpoint hs, cen, p, p01, p10;
	Flt rad, r;
	int which;
	if(s->p[0].x>s->p[1].x){
		r=s->p[0].x;
		s->p[0].x=s->p[1].x;
		s->p[1].x=r;
	}
	if(s->p[0].y>s->p[1].y){
		r=s->p[0].y;
		s->p[0].y=s->p[1].y;
		s->p[1].y=r;
	}
	cen=dmidpt(s->p[0], s->p[1]);
	hs=dsub(s->p[1], cen);
	p=Dpt(cen.x-hs.x, cen.y-hs.y);   rad=dist(arg[0], p);       which=0;
	p=Dpt(cen.x-hs.x, cen.y     ); if((r=dist(arg[0], p))<rad){ which=1; rad=r; }
	p=Dpt(cen.x-hs.x, cen.y+hs.y); if((r=dist(arg[0], p))<rad){ which=2; rad=r; }
	p=Dpt(cen.x,      cen.y-hs.y); if((r=dist(arg[0], p))<rad){ which=3; rad=r; }
	p=    cen                    ; if((r=dist(arg[0], p))<rad){ which=4; rad=r; }
	p=Dpt(cen.x,      cen.y+hs.y); if((r=dist(arg[0], p))<rad){ which=5; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y-hs.y); if((r=dist(arg[0], p))<rad){ which=6; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y     ); if((r=dist(arg[0], p))<rad){ which=7; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y+hs.y); if((r=dist(arg[0], p))<rad){ which=8; rad=r; }
	p01=Dpt(s->p[0].x, s->p[1].y);
	p10=Dpt(s->p[1].x, s->p[0].y);
	msg("case %d", which);
	switch(which){
	case 0:		/* s->p[1] fixed */
		hotline(s->p[1], left(s->p[1]), L);
		hotline(s->p[1], down(s->p[1]), L);
		track(movep, 0, s);
		break;
	case 1:		/* p10, s->p[1] fixed */
		hotline(p10, s->p[1], L|R);
		hotline(p10, down(p01), L);
		hotline(s->p[1], down(s->p[1]), L);
		track(movex0, 0, s);
		break;
	case 2:		/* p10 fixed */
		hotline(p10, right(p01), L);
		hotline(p10, down(p01), L);
		track(movex0y1, 0, s);
		break;
	case 3:		/* p01, s->p[1] fixed */
		hotline(p01, s->p[1], L|R);
		hotline(p01, left(p10), L);
		hotline(s->p[1], left(s->p[1]), L);
		track(movey0, 0, s);
		break;
	case 4:		/* all moving */
		track(moveall, 0, s);
		break;
	case 5:		/* p10, s->p[0] fixed */
		hotline(p10, s->p[0], L|R);
		hotline(p10, left(p01), L);
		hotline(s->p[0], left(s->p[0]), L);
		track(movey1, 0, s);
		break;
	case 6:		/* p01 fixed */
		hotline(p01, left(p10), L);
		hotline(p01, up(p10), L);
		track(movex1y0, 0, s);
		break;
	case 7:		/* p01, s->p[0] fixed */
		hotline(p01, s->p[0], L|R);
		hotline(p01, up(p10), L);
		hotline(s->p[0], up(s->p[0]), L);
		track(movex1, 0, s);
		break;
	case 8:		/* s->p[0] fixed */
		hotline(s->p[0], right(s->p[0]), L);
		hotline(s->p[0], up(s->p[0]), L);
		track(movep, 1, s);
		break;
	}
}
void translatebox(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
	ip->p[1]=dadd(ip->p[1], delta);
}
void deletebox(Item *ip){
}
void writebox(Item *ip, int f){
	fprint(f, "b %.3f %.3f %.3f %.3f\n", ip->p[0].x, ip->p[0].y, ip->p[1].x, ip->p[1].y);
}
void activatebox(Item *ip){
	hotline(ip->p[0], Dpt(ip->p[0].x, ip->p[1].y), L|R);
	hotline(Dpt(ip->p[0].x, ip->p[1].y), ip->p[1], L|R);
	hotline(ip->p[1], Dpt(ip->p[1].x, ip->p[0].y), L|R);
	hotline(Dpt(ip->p[1].x, ip->p[0].y), ip->p[0], L|R);
}
/*
 * This should really check that the boundary of ip intersects r.
 * Now it checks that the interior of ip intersects r.
 */
int inboxbox(Item *ip, Drectangle r){
	return drectxrect(drcanon(Drpt(ip->p[0], ip->p[1])), r);
}
Drectangle bboxbox(Item *ip){
	return drcanon(Drpt(ip->p[0], ip->p[1]));
}
Dpoint nearvertbox(Item *ip, Dpoint testp){
	Dpoint hs, cen, p, close;
	Flt rad, r;
	cen=dmidpt(ip->p[0], ip->p[1]);
	hs=dsub(ip->p[1], cen);
	p=Dpt(cen.x-hs.x, cen.y-hs.y);   rad=dist(testp, p);       close=p;
	p=Dpt(cen.x-hs.x, cen.y     ); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x-hs.x, cen.y+hs.y); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x,      cen.y-hs.y); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=    cen                    ; if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x,      cen.y+hs.y); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y-hs.y); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y     ); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	p=Dpt(cen.x+hs.x, cen.y+hs.y); if((r=dist(testp, p))<rad){ close=p; rad=r; }
	return close;
}
Itemfns boxfns={
	deletebox,
	writebox,
	activatebox,
	nearbox,
	drawbox,
	editbox,
	translatebox,
	inboxbox,
	bboxbox,
	nearvertbox,
};
Item *addbox(Item *head, Dpoint p0, Dpoint p1){
	return additem(head, BOX, 0., 0, 0, 0, &boxfns, 2, p0, p1);
}
