#include	<u.h>
#include	<libc.h>
#include	<libg.h>

/*
	stolen from libplot
*/
/*
 * fill -- polygon tiler
 * Updating the edgelist from scanline to scanline could be quicker if no
 * edges cross:  we can just merge the incoming edges.  If the scan-line
 * filling routine were a parameter, we could do textured
 * polygons, polyblt, and other such stuff.
 */
#define		SCX(x)	(x)
#define		SCY(y)	(y)

typedef enum{
	Odd=1,
	Nonzero=~0
}Windrule;
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
static void insert(Edge *ep, Edge **yp){
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
static void polygon(Bitmap *bp, int cnt[], double *pts[], Windrule w, int v, Fcode f){
	Edge *edges, *ep, *nextep, **ylist, **eylist, **yp;
	Point p, q, p0, p1, p10;
	int i, dy, nbig, y, left, right, wind, nwind, nvert;
	int *cntp;
	double **ptsp, *xp;
	nvert=0;
	for(cntp=cnt;*cntp;cntp++) nvert+=*cntp;
	edges=(Edge *)malloc(nvert*sizeof(Edge));
	if(edges==0){
	NoSpace:
		fprint(2, "fill malloc error\n");
		exits("fill malloc failure");
	}
	ylist=(Edge **)malloc((screen.r.max.y-screen.r.min.y)*sizeof(Edge *));
	if(ylist==0) goto NoSpace;
	eylist=ylist+(screen.r.max.y-screen.r.min.y);
	for(yp=ylist;yp!=eylist;yp++) *yp=0;
	ep=edges;
	for(cntp=cnt,ptsp=pts;*cntp;cntp++,ptsp++){
		p.x=SCX((*ptsp)[*cntp*2-2]);
		p.y=SCY((*ptsp)[*cntp*2-1]);
		nvert=*cntp;
		for(xp=*ptsp,i=0;i!=nvert;xp+=2,i++){
			q=p;
			p.x=SCX(xp[0]);
			p.y=SCY(xp[1]);
			if(p.y==q.y) continue;
			if(p.y<q.y){
				p0=p;
				p1=q;
				ep->dwind=1;
			}
			else{
				p0=q;
				p1=p;
				ep->dwind=-1;
			}
			if(p1.y<=screen.r.min.y) continue;
			if(p0.y>=screen.r.max.y) continue;
			ep->p=p0;
			if(p1.y>screen.r.max.y)
				ep->maxy=screen.r.max.y;
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
			if(ep->p.y<screen.r.min.y){
				dy=screen.r.min.y-ep->p.y;
				ep->x+=dy*ep->num;
				nbig=ep->x/ep->den;
				ep->p.x+=ep->dx1*nbig+ep->dx*(dy-nbig);
				ep->x%=ep->den;
				ep->p.y=screen.r.min.y;
			}
			insert(ep, ylist+(ep->p.y-screen.r.min.y));
			ep++;
		}
	}
	left = 0;
	for(yp=ylist,y=screen.r.min.y;yp!=eylist;yp++,y++){
		wind=0;
		for(ep=*yp;ep;ep=nextep){
			nwind=wind+ep->dwind;
			if(nwind&w){	/* inside */
				if(!(wind&w)){
					left=ep->p.x;
					if(left<screen.r.min.x) left=screen.r.min.x;
				}
			}
			else if(wind&w){
				right=ep->p.x;
				if(right>=screen.r.max.x) right=screen.r.max.x;
#ifdef BART_BUG_FIXED
				if(right>left)
					segment(bp, Pt(left, y), Pt(right, y), v, f);
#else
				if(right>left){
					switch(v){
					default:
						segment(bp, Pt(left, y), Pt(right, y),
							~0, D&~S);
						segment(bp, Pt(left, y), Pt(right, y),
							v, f);
						break;
					case 0:
						segment(bp, Pt(left, y), Pt(right, y),
							~0, D&~S);
						break;
					case 3:
						segment(bp, Pt(left, y), Pt(right, y),
							v, f);
						break;
					}
				}
#endif
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
}

void polyfill(Bitmap *bp, int n, Point *p, int col)
{
	int cnt[2], i;
	double v[1000], *f, *vv[1];

	cnt[0] = n;
	cnt[1] = 0;
	for(i = 0, f = v; i < n; i++, p++){
		*f++ = p->x;
		*f++ = p->y;
	}
	vv[0] = v;
	polygon(bp, cnt, vv, Odd, col, DorS);
}
