/* Copyright 1985 Massachusetts Institute of Technology */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#define add addpt
#define sub subpt
int wind = 1;

typedef enum{
	Odd=1,
	Nonzero=~0
}Windrule;
#include "catback.p"
#include "eyes.p"
#define CATWID	150		/* width of body bitmap */
#define CATHGT	300		/* height of body bitmap */
#define	TAILWID	150		/* width of tail bitmap */
#define	TAILHGT	89		/* height of tail bitmap */
#define	MINULEN	27		/* length of minute hand */
#define	HOURLEN	15		/* length of hour hand */
#define	HANDWID	4		/* width of clock hands */
#define	UPDATE	(1000/NTAIL)	/* ms/update -- tail waves at roughly 1/2 hz */
#define	BLACK	(~0)
#define	WHITE	0
#define NTP	7
Point tp[NTP]={			/* tail polygon */
	 0, 0,
	 0,76,
	 3,82,
	10,84,
	18,82,
	21,76,
	21,70,
};
#define	NTAIL	16
Image *eye[NTAIL+1];
Image *tail[NTAIL+1];
Image *cat;			/* cat body */
Image *eyes;			/* eye background */
Point toffs={ 74, -15 };	/* tail polygon offset */
Point tailoffs={0, 211};	/* tail bitmap offset, relative to body */
Point eyeoffs={49, 30};		/* eye bitmap offset, relative to body */
Point catoffs;			/* cat offset, relative to screen */
int xredraw;
int crosseyed;
void drawclock(void);
void drawhand(int, int, double);
void init(void);
Image *draweye(double);
Image *drawtail(double);
Image *eballoc(Rectangle, int);
//int myfillpoly(Image *, Point [], int, Windrule, int, Fcode);
//void mydrawpoly(Image *, Point [], int, int, Fcode);
Image *eballoc(Rectangle r, int chan){
	Image *b=allocimage(display, r, chan, 0, DWhite);
	if(b==0){
		fprint(2, "catclock: can't allocate bitmap\n");
		exits("allocimage");
	}
	return b;
}

void
eloadimage(Image *i, Rectangle r, uchar *d, int nd)
{
	int n;
	n = loadimage(i, r, d, nd);
	if(n < nd) {
		fprint(2, "loadimage fails: %r\n");
		exits("loadimage");
	}
}

int round(double x){
	return x>=0.?x+.5:x-.5;
}

void
redraw(Image *screen)
{
	Rectangle r = Rect(0,0,Dx(screen->r), Dy(screen->r));
	catoffs.x=(Dx(r)-CATWID)/2;
	catoffs.y=(Dy(r)-CATHGT)/2;
	if(!ptinrect(catoffs, r)) fprint(2, "catclock: window too small, resize!\n");
	xredraw=1;
}

void
eresized(int new)
{
	if(new && getwindow(display, Refmesg) < 0)
		fprint(2,"can't reattach to window");
	redraw(screen);
}

void main(int argc, char *argv[]){
	int i;
	ARGBEGIN{
	case 'c': crosseyed++; break;
	default:
		fprint(2, "Usage: %s [-c]\n", argv0);
		exits("usage");
	}ARGEND
	initdraw(0, 0, "cat clock");
	einit(Emouse);
	redraw(screen);
	for(i=0; i<nelem(catback_bits); i++)
		catback_bits[i] ^= 0xFF;
	for(i=0; i<nelem(eyes_bits); i++)
		eyes_bits[i] ^= 0xFF;
	cat=eballoc(Rect(0, 0, CATWID, CATHGT), GREY1);
	eloadimage(cat, cat->r, catback_bits, sizeof(catback_bits));
//	wrbitmap(cat, cat->r.min.y, cat->r.max.y, catback_bits);
	for(i=0;i<=NTAIL;i++){
		tail[i]=drawtail(i*PI/NTAIL);
		eye[i]=draweye(i*PI/NTAIL);
	}
	for(;;){
		if(ecanmouse()) emouse();	/* don't get resize events without this! */
		drawclock();
		flushimage(display, 1);
//		bflush();
		sleep(UPDATE);
	}
}
/*
 * Draw a clock hand, theta is clockwise angle from noon
 */
void drawhand(int length, int width, double theta){
	double c=cos(theta), s=sin(theta);
	double ws=width*s, wc=width*c;
	Point vhand[4];
	vhand[0]=add(screen->r.min, add(catoffs, Pt(CATWID/2+round(length*s), CATHGT/2-round(length*c))));
	vhand[1]=add(screen->r.min, add(catoffs, Pt(CATWID/2-round(ws+wc), CATHGT/2+round(wc-ws))));
	vhand[2]=add(screen->r.min, add(catoffs, Pt(CATWID/2-round(ws-wc), CATHGT/2+round(wc+ws))));
	vhand[3] = vhand[0];
	fillpoly(screen, vhand, 4, wind, display->white, 
		addpt(screen->r.min, vhand[0]));
	poly(screen, vhand, 4, Endsquare, Endsquare, 0, display->black,
		addpt(screen->r.min, vhand[0]));
//	myfillpoly(&screen, vhand, 3, Nonzero, WHITE, S);
//	mydrawpoly(&screen, vhand, 3, BLACK, S);
}
/*
 * draw a cat tail, t is time (mod 1 second)
 */
Image *drawtail(double t){
	Image *bp;
	double theta=.4*sin(t+3.*PIO2)-.08;	/* an assymetric tail leans to one side */
	double s=sin(theta), c=cos(theta);
	Point rtp[NTP];
	int i;
	bp=eballoc(Rect(0, 0, TAILWID, TAILHGT), GREY1);
	for(i=0;i!=NTP;i++)
		rtp[i]=add(Pt(tp[i].x*c+tp[i].y*s, -tp[i].x*s+tp[i].y*c), toffs);
	fillpoly(bp, rtp, NTP, wind, display->black, rtp[0]);
	return bp;
}
/*
 * draw the cat's eyes, t is time (mod 1 second)
 */
Image *draweye(double t){
	Image *bp;
	double u;
	double angle=0.7*sin(t+3*PIO2)+PI/2.0;	/* direction eyes point */
	Point pts[100];
	int i, j;
	struct{
		double	x, y, z;
	}pt;
	if(eyes==0){
		eyes=eballoc(Rect(0, 0, eyes_width, eyes_height), GREY1);
		eloadimage(eyes, eyes->r, eyes_bits, sizeof(eyes_bits));
//		wrbitmap(eyes, eyes->r.min.y, eyes->r.max.y, eyes_bits);
	}
	bp=eballoc(eyes->r, GREY1);
	draw(bp, bp->r, eyes, nil, ZP);
//	bitblt(bp, bp->r.min, eyes, eyes->r, S);
	for(i=0,u=-PI/2.0;u<PI/2.0;i++,u+=0.25){
		pt.x=cos(u)*cos(angle+PI/7.0);
		pt.y=sin(u);
		pt.z=2.+cos(u)*sin(angle+PI/7.0);
		pts[i].x=(pt.z==0.0?pt.x:pt.x/pt.z)*23.0+12.0;
		pts[i].y=(pt.z==0.0?pt.y:pt.y/pt.z)*23.0+11.0;
	}
	for(u=PI/2.0;u>-PI/2.0;i++,u-=0.25){
		pt.x=cos(u)*cos(angle-PI/7.0);
		pt.y=sin(u);
		pt.z=2.+cos(u)*sin(angle-PI/7.0);
		pts[i].x=(pt.z==0.0?pt.x:pt.x/pt.z)*23.0+12.0;
		pts[i].y=(pt.z==0.0?pt.y:pt.y/pt.z)*23.0+11.0;
	}
	fillpoly(bp, pts, i, wind, display->black, pts[0]);
//	fillpoly(bp, pts, i, Nonzero, BLACK, S);
	if(crosseyed){
		angle=0.7*sin(PI-t+3*PIO2)+PI/2.0;
		for(i=0,u=-PI/2.0;u<PI/2.0;i++,u+=0.25){
			pt.x=cos(u)*cos(angle+PI/7.0);
			pt.y=sin(u);
			pt.z=2.+cos(u)*sin(angle+PI/7.0);
			pts[i].x=(pt.z==0.0?pt.x:pt.x/pt.z)*23.0+12.0;
			pts[i].y=(pt.z==0.0?pt.y:pt.y/pt.z)*23.0+11.0;
		}
		for(u=PI/2.0;u>-PI/2.0;i++,u-=0.25){
			pt.x=cos(u)*cos(angle-PI/7.0);
			pt.y=sin(u);
			pt.z=2.+cos(u)*sin(angle-PI/7.0);
			pts[i].x=(pt.z==0.0?pt.x:pt.x/pt.z)*23.0+12.0;
			pts[i].y=(pt.z==0.0?pt.y:pt.y/pt.z)*23.0+11.0;
		}
	}
	for(j=0;j<i;j++) pts[j].x+=31;
	fillpoly(bp, pts, i, wind, display->black, pts[0]);
//	fillpoly(bp, pts, i, Nonzero, BLACK, S);
	return bp;
}
void
drawclock(void){
	static int t=0, dt=1;
	static Tm otm;
	Tm tm=*localtime(time(0));
	tm.hour%=12;
	if(xredraw || tm.min!=otm.min || tm.hour!=otm.hour){
		if(xredraw){
			draw(screen, screen->r, display->white, nil, ZP);
			border(screen, screen->r, 4, display->black, ZP);
			//bitblt(&screen, screen.r.min, &screen, screen.r, Zero);
			//border(&screen, screen.r, 4, F);
		}
		draw(screen, screen->r, cat, nil, mulpt(catoffs, -1));
		flushimage(display, 1);
		//bitblt(&screen, catoffs, cat, cat->r, S);
		drawhand(MINULEN, HANDWID, 2.*PI*tm.min/60.);
		drawhand(HOURLEN, HANDWID, 2.*PI*(tm.hour+tm.min/60.)/12.);
		xredraw=0;
	}
	draw(screen, screen->r, tail[t], nil, 
		mulpt(add(catoffs, tailoffs), -1));
	draw(screen, screen->r, eye[t], nil, 
		mulpt(add(catoffs, eyeoffs), -1));
	//bitblt(&screen, add(catoffs, tailoffs), tail[t], tail[t]->r, S);
	//bitblt(&screen, add(catoffs, eyeoffs), eye[t], eye[t]->r, S);
	t+=dt;
	if(t<0 || t>NTAIL){
		t-=2*dt;
		dt=-dt;
	}
	otm=tm;
}
#ifdef NOTDEF
void drawpoly(Bitmap *dst, Point p[], int np, int v, Fcode f){
	int i;
	Point q=p[np-1];
	for(i=0;i!=np;i++){
		segment(dst, p[i], q, v, f);
		q=p[i];
	}
}
/*
 * Fillpoly -- a polygon tiler
 * Updating the edgelist from scanline to scanline could be quicker if no
 * edges cross:  we can just merge the incoming edges.  The code can handle
 * multiply-connected polygons with holes, but the interface can't.  If
 * the scan-line filling routine were a parameter, we could do textured
 * polygons, polyblt, and other such stuff.
 */
typedef struct edge Edge;
struct edge{
	Point p;	/* point of crossing current scan-line */
	int maxy;	/* scan line at which to discard edge */
	int dx;		/* x increment if x fraction<1 */
	int dx1;	/* x increment if x fraction>=1 */
	int x;		/* x fraction, scaled by den */
	int num;	/* x fraction increment for unit y change, scaled by den */
	int den;	/* x fraction increment for unit x change, scaled by num */
	int dwind;	/* increment of winding number on passing this edge */
	Edge *next;	/* next edge on current scanline */
	Edge *prev;	/* previous edge on current scanline */
};
void insert(Edge *ep, Edge **yp){
	while(*yp && (*yp)->p.x<ep->p.x) yp=&(*yp)->next;
	ep->next=*yp;
	*yp=ep;
	if(ep->next){
		ep->prev=ep->next->prev;
		ep->next->prev=ep;
		if(ep->prev)
			ep->prev->next=ep;
	}
	else
		ep->prev=0;
}
int myfillpoly(Bitmap *b, Point vert[], int nvert, Windrule w, int v, Fcode f){
	Edge *edges, *ep, *nextep, **ylist, **eylist, **yp;
	Point *p, *q, *evert, p0, p1, p10;
	int dy, nbig, y, left, right, wind, nwind;
	edges=(Edge *)malloc(nvert*sizeof(Edge));
	if(edges==0){
	NoSpace:
		return 0;
	}
	ylist=(Edge **)malloc((b->r.max.y-b->r.min.y)*sizeof(Edge *));
	if(ylist==0) goto NoSpace;
	eylist=ylist+(b->r.max.y-b->r.min.y);
	for(yp=ylist;yp!=eylist;yp++) *yp=0;
	evert=vert+nvert;
	for(p=evert-1, q=vert, ep=edges;q!=evert;p=q, q++, ep++){
		if(p->y==q->y) continue;
		if(p->y<q->y){
			p0=*p;
			p1=*q;
			ep->dwind=1;
		}
		else{
			p0=*q;
			p1=*p;
			ep->dwind=-1;
		}
		if(p1.y<=b->r.min.y) continue;
		if(p0.y>=b->r.max.y) continue;
		ep->p=p0;
		if(p1.y>b->r.max.y)
			ep->maxy=b->r.max.y;
		else
			ep->maxy=p1.y;
		p10=sub(p1, p0);
		if(p10.x>=0){
			ep->dx=p10.x/p10.y;
			ep->dx1=ep->dx+1;
		}
		else{
			p10.x=-p10.x;
			ep->dx=-(p10.x/p10.y); /* this nonsense rounds toward zero */
			ep->dx1=ep->dx-1;
		}
		ep->x=0;
		ep->num=p10.x%p10.y;
		ep->den=p10.y;
		if(ep->p.y<b->r.min.y){
			dy=b->r.min.y-ep->p.y;
			ep->x+=dy*ep->num;
			nbig=ep->x/ep->den;
			ep->p.x+=ep->dx1*nbig+ep->dx*(dy-nbig);
			ep->x%=ep->den;
			ep->p.y=b->r.min.y;
		}
		insert(ep, ylist+(ep->p.y-b->r.min.y));
	}
	left=0;
	for(yp=ylist,y=b->r.min.y;yp!=eylist;yp++,y++){
		wind=0;
		for(ep=*yp;ep;ep=nextep){
			nwind=wind+ep->dwind;
			if(nwind&w){	/* inside */
				if(!(wind&w)){
					left=ep->p.x;
					if(left<b->r.min.x) left=b->r.min.x;
				}
			}
			else if(wind&w){
				right=ep->p.x;
				if(right>=b->r.max.x) right=b->r.max.x;
				if(right>left)
					segment(b, Pt(left, y), Pt(right, y), v, f);
			}
			wind=nwind;
			nextep=ep->next;
			if(++ep->p.y!=ep->maxy){
				ep->x+=ep->num;
				if(ep->x>=ep->den){
					ep->x-=ep->den;
					ep->p.x+=ep->dx1;
				}
				else
					ep->p.x+=ep->dx;
				insert(ep, yp+1);
			}
		}
	}
	free((char *)edges);
	free((char *)ylist);
	return 1;
}
#endif
