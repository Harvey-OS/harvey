#include "art.h"
/*
 * code to maintain alignment data structure (global variable active).
 */
void activateprim(Item *ip, Item *op){
	if(op->flags&HOT) (*ip->fn->activate)(ip);
}
void activate(Item *ip){
	walk(ip, scoffs, activateprim);
}
void hotline(Dpoint p, Dpoint q, int ep){
	Dpoint d, r;
	int i;
	Flt ang, l;
	if(ep&L) hotpoint(p);
	if(ep&R) hotpoint(q);
	d=dsub(q, p);
	l=dlen(d);
	if(l<SMALL){ msg("tiny line"); return; }
	ang=datan2(d.y, d.x);
	for(i=0;i!=nalign[ANGLE];i++) if(align[ANGLE][i].active){
		if(ep&L) alline(p, ang+align[ANGLE][i].value);
		if(ep&R) alline(q, ang+align[ANGLE][i].value);
	}
	for(i=0;i!=nalign[PARA];i++) if(align[PARA][i].active){
		r.x=d.y*align[PARA][i].value/l;
		r.y=-d.x*align[PARA][i].value/l;
		alline(dadd(p, r), ang);
		alline(dsub(p, r), ang);
	}
}
void hotarc(Dpoint p, Dpoint q, Dpoint r){
	Dpoint cen, d;
	int i;
	Flt pang, rang;
	if(pldist(p, q, r)<CLOSE){
		hotline(p, r, L|R);
		return;
	}
	hotpoint(p);
	hotpoint(q);
	cen=circumcenter(p, q, r);
	d=dsub(p, cen);
	pang=datan2(d.x, -d.y);
	d=dsub(r, cen);
	rang=datan2(d.x, -d.y);
	for(i=0;i!=nalign[ANGLE];i++) if(align[ANGLE][i].active){
		alline(p, pang+align[ANGLE][i].value);
		alline(r, rang+align[ANGLE][i].value);
	}
}
void hotpoint(Dpoint p){
	int i;
	for(i=0;i!=nalign[SLOPE];i++) if(align[SLOPE][i].active)
		alline(p, align[SLOPE][i].value);
	for(i=0;i!=nalign[CIRC];i++)
		if(align[CIRC][i].active)
			alcirc(p, align[CIRC][i].value);
}
/*
 * add an alignment line through p at angle t
 */
void alline(Dpoint p, Flt t){
	Dpoint p0, p1, d;
	Item *ip;
	d.x=2000.*cos(radians(t));
	d.y=2000.*sin(radians(t));
	p0=dsub(p, d);
	p1=dadd(p, d);
	if(dclipline(dwgdrect, &p0, &p1) && !findline(active, p0, p1)){
		ip=addline(active, p0, p1);
		draw(ip, Dpt(0., 0.), FAINT, S|D);
	}
}
/*
 * add an alignment circle through p with radius r
 */
void alcirc(Dpoint p, Flt r){
	Item *ip;
	if(!findcircle(active, p, r)){
		ip=addcircle(active, p, r);
		draw(ip, Dpt(0., 0.), FAINT, S|D);
	}
}
int findline(Item *ip, Dpoint p, Dpoint q){
	Item *cp;
	switch(ip->type){
	default: return 0;
	case LINE:
		return deqpt(ip->p[0], p) && deqpt(ip->p[1], q)
		    || deqpt(ip->p[0], q) && deqpt(ip->p[1], p);
	case HEAD:
		for(cp=ip->next;cp!=ip;cp=cp->next)
			if(findline(cp, p, q)) return 1;
		return 0;
	}
}
int findcircle(Item *ip, Dpoint p, Flt r){
	Item *cp;
	switch(ip->type){
	default:
		return 0;
	case CIRCLE:
		return deqpt(ip->p[0], p) && same(ip->r, r);
	case HEAD:
		for(cp=ip->next;cp!=ip;cp=cp->next)
			if(findcircle(cp, p, r)) return 1;
		return 0;
	}
}
#define	code(p)	((p->x<xl?1:p->x>xr?2:0)|(p->y>yt?4:p->y<yb?8:0))
#define	xl r.min.x
#define	yb r.min.y
#define	xr r.max.x
#define yt r.max.y
#define	x0 p0->x
#define	y0 p0->y
#define	x1 p1->x
#define	y1 p1->y
int dclipline(Drectangle r, Dpoint *p0, Dpoint *p1){
	int c0=code(p0), c1=code(p1), c;
	Dpoint t;
	while(c0|c1){
		if(c0&c1) return 0;
		c=c0?c0:c1;
		if(c&1)     t.y=y0+(y1-y0)*(xl-x0)/(x1-x0), t.x=xl;
		else if(c&2)t.y=y0+(y1-y0)*(xr-x0)/(x1-x0), t.x=xr;
		else if(c&4)t.x=x0+(x1-x0)*(yt-y0)/(y1-y0), t.y=yt;
		else        t.x=x0+(x1-x0)*(yb-y0)/(y1-y0), t.y=yb;
		if(c==c0) *p0=t, c0=code(p0);
		else      *p1=t, c1=code(p1);
	}
	return 1;
}
void Oheat(void){
	if(selection){
		heat(selection);
		activate(selection);
	}
	else msg("heat: nothing selected");
}
/*
 * turn a's HOT flags on
 */
void heat(Item *a){
	Item *ip;
	if(a->type==HEAD)
		for(ip=a->next;ip!=a;ip=ip->next)
			heat(ip);
	else a->flags|=HOT;
}
void Ocoolall(void){
	cool(scene);
	delete(active);
	active=addhead();
	redraw();
	reheat();
}
/*
 * turn a's HOT flags off
 */
void cool(Item *a){
	Item *p;
	a->flags&=~HOT;
	if(a->type==HEAD){
		for(p=a->next;p!=a;p=p->next)
			cool(p);
	}
}
/*
 * get intersections of two objects
 * Missing cases:
 *	spline-circle
 *	spline-arc
 */
int itemxitem(Item *p, Item *q, Dpoint *i){
	Item r, *t;
	int n;
	Dpoint pt[2];
	if(p==q && p->type!=SPLINE) return 0;
	if(q->type==BOX){
		t=p;
		p=q;
		q=t;
	}
	switch(p->type){
	case BOX:
		r.type=LINE;
		r.p=pt;
		r.np=2;
		r.p[0]=p->p[0];
		r.p[1]=Dpt(p->p[0].x, p->p[1].y);
		n=itemxitem(q, &r, i);
		r.p[0]=p->p[1];
		n+=itemxitem(q, &r, i+n);
		r.p[1]=Dpt(p->p[1].x, p->p[0].y);
		n+=itemxitem(q, &r, i+n);
		r.p[0]=p->p[0];
		return n+itemxitem(q, &r, i+n);
	case LINE:
		switch(q->type){
		case LINE:	return seginterseg(p->p[0], p->p[1], q->p[0], q->p[1], i);
		case CIRCLE:	return segintercircle(p->p[0], p->p[1], q->p[0], q->r, i);
		case ARC:	return seginterarc(p->p[0], p->p[1], q->p[0], q->p[1], q->p[2], i);
		case SPLINE:	return seginterspline(p->p[0], p->p[1], q, i);
		}
		break;
	case CIRCLE:
		switch(q->type){
		case LINE:	return segintercircle(q->p[0], q->p[1], p->p[0], p->r, i);
		case CIRCLE:	return circintercirc(p->p[0], p->r, q->p[0], q->r, i);
		case ARC:	return circinterarc(p->p[0], p->r, q->p[0], q->p[1], q->p[2], i);
		}
		break;
	case ARC:
		switch(q->type){
		case LINE:	return seginterarc(q->p[0], q->p[1], p->p[0], p->p[1], p->p[2], i);
		case CIRCLE:	return circinterarc(q->p[0], q->r, p->p[0], p->p[1], p->p[2], i);
		case ARC:	return arcinterarc(p->p[0],p->p[1],p->p[2], q->p[0],q->p[1],q->p[2], i);
		}
	case SPLINE:
		switch(q->type){
		case LINE:	return seginterspline(q->p[0], q->p[1], p, i);
		case SPLINE:	return splineinterspline(p, q, i);
		}
	}
	return 0;	/* missing cases behave as if no intersection */
}
void reheat(void){
	delete(active);
	active=addhead();
	redraw();
	activate(scene);
}
void flatten(Item *ip, Item *op){
	ip=additemv(active, ip->type, ip->r, ip->face, ip->text, ip->group, ip->fn,
		ip->np, ip->p, Dpt(0., 0.));
	ip->flags|=FLAT|INVIS;	/* INVIS because the originals are already visible */
	ip->orig=op;
}
void realign(void){
	Item *ip, *next;
	for(ip=active->next;ip!=active;ip=next){
		next=ip->next;
		if(ip->flags&FLAT) delete(ip);
	}
	mwalk(scene, scoffs, flatten);
}
