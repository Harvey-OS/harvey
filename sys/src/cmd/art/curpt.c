/*
 * Routines to track the current point
 */
#include "art.h"
Dpoint curpt;
void movenone(Item *ip, int index, Dpoint p){
}
void track(void (*rou)(Item *, int, Dpoint), int index, Item *ip){
	Dpoint p;
	Item *s;
	if(ip){
		setselection(0);
		draw(ip, scoffs, dark, S^D);
		ip->flags|=MOVING;
	}
	realign();
	svpoint=curpt;
	while(button1()){
		p=curpt;
		mvcurpt(P2D(mouse.xy));
		if(!deqpt(p, curpt))
			(*rou)(ip, index, curpt);
		getmouse();
	}
	if(ip){
		ip->flags&=~MOVING;
		draw(ip, scoffs, dark, S^D);
		setselection(ip);
	}
	else setselection(select());
}
/*
 * Change curpt to be p, as modified by gravity.
 * How gravity works:
 *	First first we look for an alignment point (on the alpt list) that is
 *	within the gravity radius.  If we find it, we snap to the point.
 *	Otherwise, we examine each item in scene and active looking for
 *	the closest one.  If that's within the gravity radius, we snap to
 *	the closest point on the item.
 *	Otherwise, curpt is unchanged.
 */
void mvcurpt(Dpoint testp){
	Item *p, *q, *near, *sel;
	Dpoint i[100], nearp, np;
	Flt rad, r;
	int n, j;
	Flt grav=gravity;
	if(grav==0.) grav=.02;	/* if no gravity, we still must select close objects */
	nearp=testp;
	rad=grav;
	near=0;
	sel=0;
	for(p=active->next;p!=active;p=p->next){
		p->nearpt=(*p->fn->near)(p, testp);
		if(dist(p->nearpt, testp)<rad){
			p->near=near;
			near=p;
			for(q=near;q;q=q->near){
				n=itemxitem(p, q, i);
				for(j=0;j!=n;j++){
					r=dist(i[j], testp);
					if(r<rad){
						rad=r;
						nearp=i[j];
					}
				}
			}
		}
		np=(*p->fn->nearvert)(p, testp);
		r=dist(np, testp);
		if(r<rad){
			rad=r;
			nearp=np;
		}
	}
	if(rad==grav){
		for(p=near;p;p=p->near){
			r=dist(p->nearpt, testp);
			if(r<rad){
				rad=r;
				nearp=p->nearpt;
			}
		}
	}
	if(rad==grav){
		np=neargrid(testp);
		if(dist(np, testp)<rad) nearp=np;
	}
	if(gravity==0.) nearp=testp;	/* no gravity -- don't stick to things */
	nearp=dsub(nearp, scoffs);
	if(!deqpt(nearp, curpt)){
		bitblt(&screen, add(D2P(dadd(nearp, scoffs)), Pt(-7, 0)), cur, cur->r, S^D);
		bitblt(&screen, add(D2P(dadd(curpt, scoffs)), Pt(-7, 0)), cur, cur->r, S^D);
		curpt=nearp;
	}
}
drawcurpt(void){
	bitblt(&screen, add(D2P(dadd(curpt, scoffs)), Pt(-7, 0)), cur, cur->r, S^D);
}
Item *select(void){
	Item *near=0, *p;
	int nclose=0;
	Dpoint testp;
	testp=dadd(curpt, scoffs);
	for(p=active->next;p!=active;p=p->next){
		if(p->flags&FLAT){
			if(dsq((*p->fn->near)(p, testp), testp)<1./(DPI*DPI)
			|| dsq((*p->fn->nearvert)(p, testp), testp)<1./(DPI*DPI)){
				p->near=near;
				near=p;
				nclose++;
			}
		}
	}
	if(near==0) return 0;
	for(p=near;p;p=p->near)
		if(p->orig==selection)
			return p->near?p->near->orig:near->orig;
	return near->orig;
}
