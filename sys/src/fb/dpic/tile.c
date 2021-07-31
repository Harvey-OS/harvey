/*
 * polygon tiler
 */
#include "ext.h"
float **image;
Edge *enter;
Rgba **page;
Span *span;
void tilestart(void){
	int i;
	initacc();
	for(i=0;i!=pagehgt+1;i++){
		enter[i].next=enter[i].prev=&enter[i];
		span[i]=(Span){pagewid-1, 0};
	}
}
void tileedge(double x0, double y0, double x1, double y1){
	Edge *e, *b;
	int y;
	if(y0<=0 && y1<=0		/* off the bottom? */
	|| pagehgt<=y0 && pagehgt<=y1	/* off the top? */
	|| (int)y0==y0 && y0==y1	/* in the crack? */
	|| x0==x1 && y0==y1)		/* too small? */
		return;
	diceedge(x0, y0, x1, y1);
	if(y0==y1) return; /* can't cross an edge, could cause divide a check */
	e=emalloc(sizeof(Edge));
	if(y0<y1){
		e->x0=x0;
		e->y0=y0;
		e->x1=x1;
		e->y1=y1;
		e->right=0;
	}
	else{
		e->x0=x1;
		e->y0=y1;
		e->x1=x0;
		e->y1=y0;
		e->right=1;
	}
	if(e->y0<1.)
		y=1;
	else
		y=floor(e->y0)+1.;
	e->dx=(e->x1-e->x0)/(e->y1-e->y0);
	e->x=e->x0+(y-e->y0)*e->dx;
	b=&enter[y];
	e->next=b;
	e->prev=b->prev;
	b->prev->next=e;
	b->prev=e;
}
void tileend(void){
	int y, x;
	float *ip, *eip, alf;
	Rgba *pp;
	Edge *e;
	Span *sp;
	interior();
	for(y=0,sp=span;y!=pagehgt;y++,sp++) if(sp->min<=sp->max){
		eip=&image[y][sp->max+1];
		for(ip=&image[y][sp->min],pp=&page[y][sp->min];ip!=eip;ip++,pp++){
			alf=*ip;
			pp->r+=alf*(color.r-pp->r);
			pp->g+=alf*(color.r-pp->g);
			pp->b+=alf*(color.r-pp->b);
			pp->a+=alf*(color.r-pp->a);
			*ip=0;
		}
	}
}
/*
 * dice the edge at pixel boundaries, accumulating the convolution
 * as we go.  Splits 4 ways to avoid confution.
 */
void diceedge(double x0, double y0, double x1, double y1){
	if(x0<x1)
		if(y0<y1)
			diceuu(x0, y0, x1, y1);
		else
			diceud(x0, y0, x1, y1);
	else if(y0<y1)
		dicedu(x0, y0, x1, y1);
	else
		dicedd(x0, y0, x1, y1);
}
diceuu(double x0, double y0, double x1, double y1){
	double nx, ynx;		/* next x crossing */
	double ydx;		/* step to subsequent x crossing */
	double ny, xny;		/* next y crossing */
	double xdy;		/* step to subsequent y crossing */
	int xc=floor(x0), yc=floor(y0);
	nx=xc+1;
	ydx=(y1-y0)/(x1-x0);
	ynx=y0+ydx*(nx-x0);
	ny=yc+1;
	xdy=(x1-x0)/(y1-y0);
	xny=x0+xdy*(ny-y0);
	for(;;){
		if(nx<xny){
			if(nx>x1 || ynx>y1) break;
			accumulate(x0, y0, nx, ynx, xc, yc);
			x0=nx;
			y0=ynx;
			xc++;
			nx+=1.;
			ynx+=ydx;
		}
		else{
			if(xny>x1 || ny>y1) break;
			accumulate(x0, y0, xny, ny, xc, yc);
			x0=xny;
			y0=ny;
			xny+=xdy;
			yc++;
			ny+=1.;
		}
	}
	accumulate(x0, y0, x1, y1, xc, yc);
}
diceud(double x0, double y0, double x1, double y1){
	double nx, ynx;		/* next x crossing */
	double ydx;		/* step to subsequent x crossing */
	double ny, xny;		/* next y crossing */
	double xdy;		/* step to subsequent y crossing */
	int xc=floor(x0), yc=floor(y0);
	nx=xc+1;
	ydx=(y1-y0)/(x1-x0);
	ynx=y0+ydx*(nx-x0);
	ny=yc;
	xdy=y0==y1?-1e30:(x1-x0)/(y1-y0);
	xny=x0+xdy*(ny-y0);
	for(;;){
		if(nx<xny){
			if(nx>x1 || ynx<y1) break;
			accumulate(x0, y0, nx, ynx, xc, yc);
			x0=nx;
			y0=ynx;
			xc++;
			nx+=1.;
			ynx+=ydx;
		}
		else{
			if(xny>x1 || ny<y1) break;
			accumulate(x0, y0, xny, ny, xc, yc);
			x0=xny;
			y0=ny;
			xny-=xdy;
			--yc;
			ny-=1.;
		}
	}
	accumulate(x0, y0, x1, y1, xc, yc);
}
dicedu(double x0, double y0, double x1, double y1){
	double nx, ynx;		/* next x crossing */
	double ydx;		/* step to subsequent x crossing */
	double ny, xny;		/* next y crossing */
	double xdy;		/* step to subsequent y crossing */
	int xc=floor(x0), yc=floor(y0);
	nx=xc;
	ydx=x0==x1?-1e30:(y1-y0)/(x1-x0);
	ynx=y0+ydx*(nx-x0);
	ny=yc+1;
	xdy=(x1-x0)/(y1-y0);
	xny=x0+xdy*(ny-y0);
	for(;;){
		if(nx>xny){
			if(nx<x1 || ynx>y1) break;
			accumulate(x0, y0, nx, ynx, xc, yc);
			x0=nx;
			y0=ynx;
			--xc;
			nx-=1.;
			ynx-=ydx;
		}
		else{
			if(xny<x1 || ny>y1) break;
			accumulate(x0, y0, xny, ny, xc, yc);
			x0=xny;
			y0=ny;
			xny+=xdy;
			yc++;
			ny+=1.;
		}
	}
	accumulate(x0, y0, x1, y1, xc, yc);
}
dicedd(double x0, double y0, double x1, double y1){
	double nx, ynx;		/* next x crossing */
	double ydx;		/* step to subsequent x crossing */
	double ny, xny;		/* next y crossing */
	double xdy;		/* step to subsequent y crossing */
	int xc=floor(x0), yc=floor(y0);
	nx=xc;
	ydx=x0==x1?1e30:(y1-y0)/(x1-x0);
	ynx=y0+ydx*(nx-x0);
	ny=yc;
	xdy=y0==y1?1e30:(x1-x0)/(y1-y0);
	xny=x0+xdy*(ny-y0);
	for(;;){
		if(nx>xny){
			if(nx<x1 || ynx<y1) break;
			accumulate(x0, y0, nx, ynx, xc, yc);
			x0=nx;
			y0=ynx;
			--xc;
			nx-=1.;
			ynx-=ydx;
		}
		else{
			if(xny<x1 || ny<y1) break;
			accumulate(x0, y0, xny, ny, xc, yc);
			x0=xny;
			y0=ny;
			xny-=xdy;
			--yc;
			ny-=1.;
		}
	}
	accumulate(x0, y0, x1, y1, xc, yc);
}
void interior(void){
	Edge *e, *new, *cp, *next;
	int y;
	int wnum;	/* winding number in this region */
	Edge curr;	/* active edges */
	curr.next=curr.prev=&curr;
	curr.right=0;
	for(y=1;y<=pagehgt;y++){
		/*
		 * delete exiting edges 
		 */
		for(e=curr.next;e!=&curr;e=e->next){
			if(e->y1<y){
				e->next->prev=e->prev;
				e->prev->next=e->next;
				free(e);
			}
		}
		/*
		 * insert newly entering edges
		 */
		new=&enter[y];
		for(e=new->next;e!=new;e=next){
			next=e->next;
			if(e->y1>=y){
				for(cp=curr.next;cp!=&curr && right(e, cp);cp=cp->next);
				e->prev=cp->prev;
				e->next=cp;
				e->next->prev=e;
				e->prev->next=e;
			}
		}
		/*
		 * accumulate top contributions
		 */
		wnum=0;
		for(e=curr.next;e->next!=&curr;e=e->next){
			if(e->right) --wnum;
			else wnum++;
			acctop(e->x, e->next->x, y-1, wnum);
		}
		/*
		 * move on to the next line
		 */
		for(e=curr.next;e!=&curr;e=e->next)
			e->x+=e->dx;
	}
	for(e=curr.next;e!=&curr;e=e->next){
		e->next->prev=e->prev;
		e->prev->next=e->next;
		free(e);
	}
}
int right(Edge *a, Edge *b){
	if(a->x!=b->x) return a->x>b->x;
	return (b->y1-b->y0)*(a->x1-a->x0)<(a->y1-a->y0)*(b->x1-b->x0);
}
addspan(int y, int x0, int x1){
	Span *sp=&span[y];
	if(x0<sp->min) sp->min=x0;
	if(x1>sp->max) sp->max=x1;
}
/*
 * These three routines (initacc, acctop and accumulate) are the only part of
 * the code that depends on the sampling kernel.
 */
initacc(void){
}
acctop(float x0, float x1, int y, int wnum){
	float *p;
	float nx;
	int ix0;
	if(x0>=pagewid || x1<=0 || wnum==0) return;
	if(x0<0) x0=0;
	if(x1>pagewid) x1=pagewid;
	ix0=x0;	/* x0>=0, so (int)x0 is (int)floor(x0) */
	p=&image[y][ix0];
	nx=ix0+1;
	addspan(y, ix0, (int)x1);
	while(nx<x1){
		*p++ += (nx-x0)*wnum;
		x0=nx;
		nx+=1.;
	}
	if(x0<pagewid)
		*p+=(x1-x0)*wnum;
}
accumulate(float x0, float y0, float x1, float y1, int x, int y){
	if(0<=x && x<pagewid && 0<=y && y<pagehgt){
		addspan(y, x, x);
		image[y][x]+=(x1-x0)*(.5*(y0+y1)-y);
	}
}
