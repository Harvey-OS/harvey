#include <u.h>
#include <libc.h>
#include "system.h"
#include "stdio.h"
#include "defines.h"
#include "object.h"
#include "path.h"
#include "graphics.h"
#include "clip.h"
#define CW 1
#define CCW 2
#define MAXMOVE 20
#define MAXPATHS 1000
#define MAXINTERS 50
#define HORIZ 1
#define VERT 2
#define NONE	0
#define LEFT	1
#define RIGHT	2
#define BOTH	3

struct lstats {
	int type;
	double slope;
	int inside, member;
	struct element *altq;
	struct lstats *prev, *next;
};
struct smoves {
	struct element *moves;
	struct lstats *sptr;
	int bpath, objct, count;
	double xmin, ymin, xmax, ymax;
	int used;
};
struct pts {
	struct element *q, *p;
	struct lstats *qs, *ps;
	double x, y, dist, qdist;
	int qcode, pcode, qint, pint, group, elt;
};
int qinterior[MAXMOVE], pinterior[MAXMOVE];
struct lstats *qst[MAXMOVE], *pst[MAXMOVE];
struct element *qelement[MAXMOVE], *pelement[MAXMOVE];
static struct pts inters[MAXPATHS], winters[MAXPATHS], *pts, *newpt;
static pct, qct;
static double minx, miny, maxx, maxy;
int pointtype;
struct lstats *saveit;

void
eoclipOP(void){
	clipOP();
}

void
clipOP(void){
	struct path oldpath;
	struct element *p;
	oldpath = copypath(Graphics.path.first);
	close_components();
	flattenpathOP();
	do_clip();
	rel_path(Graphics.clippath.first);
	Graphics.clippath = Graphics.path;
	clean_path(&Graphics.clippath);
	for(p=Graphics.clippath.first; p != NULL; p=p->next){
		if(p->type == PA_MOVETO)continue;
		if((fabs(p->prev->ppoint.x-p->ppoint.x) > SMALLDIF) &&
			(fabs(p->prev->ppoint.y-p->ppoint.y) > SMALLDIF)){
			Graphics.clipchanged = 1;
			break;
		}
	}
	Graphics.iminx = (int)minx; Graphics.iminy = (int)miny;
	Graphics.imaxx = (int)maxx; Graphics.imaxy = (int)maxy;
	Graphics.path = oldpath;
}
static int inoutline;

void
outlineOP(void){
	inoutline = 1;
	clipOP();
	inoutline = 0;
}

static int copied;
int
do_clip(void){
	struct lstats pstats[MAXPATHS], qstats[MAXPATHS];
	struct smoves pmoves[MAXMOVE], qmoves[MAXMOVE], *pm, *qm;
	struct element *p, *lastp, *q, *save_el;
	struct lstats *ps;
	struct path clpath;
	double deltax;
	int k,j, rcode=0, pmove, qmove, movect=0, qin, pin, pint;

	copied = 0;
	minx = (double)Graphics.imaxx;
	miny = (double)Graphics.imaxy;
	maxx = (double)Graphics.iminx;
	maxy = (double)Graphics.iminy;
	pointtype = PA_MOVETO;
	if(Graphics.path.first == NULL)return(0);
	clpath = copypath(Graphics.path.first);
	clean_path(&clpath);
	if(clpath.first->next == clpath.last)return(1);
	if(mdebug)fprintf(stderr,"/Times-Bold findfont 10 scalefont setfont\n/filename(db.ps)def filename run\n");
	pmove = getslope(&clpath, pstats, pmoves, pinterior, pst, pelement);
	qmove = getslope(&Graphics.clippath, qstats, qmoves, qinterior, qst, qelement);
	if(mdebug){
		fprintf(stderr,"%%pmove %d qmove %d\n",pmove,qmove);
		printpath("clippath",Graphics.clippath, 1000, 0);
		printpath("path to clip",clpath, 1000, 0);
	}
	newpathOP();
	for(pm=pmoves; pm < &pmoves[pmove]; pm++){
	   pts = winters;
	   for(qm=qmoves; qm < &qmoves[qmove]; qm++){
		if(mdebug)fprintf(stderr,"%%Part %d Qpart %d\n",pm-pmoves,qm-qmoves);
		qct = qm->count;
		pct = pm->count;
		if(mdebug)fprintf(stderr,"%%qmct %d pmct %d\n",qm->objct,pm->objct);
		qin = getinside(qm->moves,qst,qm->objct,qm->bpath,
			pm->moves,pst, pm->objct, pm->bpath, pinterior,
			pm->xmin,pm->ymin,pm->xmax, pm->ymax);
		pin = getinside(pm->moves,pst,pm->objct,pm->bpath,
			qm->moves,qst, qm->objct, qm->bpath,qinterior,
			qm->xmin,qm->ymin,qm->xmax, qm->ymax);
		if(mdebug){
		if(mdebug)fprintf(stderr,"/Times-Roman findfont 8 scalefont setfont\n");
		fprintf(stderr,"%%qct %d pct %d qin %d pin %d qused %d\n",qct, pct,
			qin, pin, qm->used);
		}
		if(qin == qct && pin == pct && (pm == &pmoves[pmove-1] && !qm->used&&
		  (pmove > 1||qm->xmin > pm->xmax||qm->ymin > pm->ymax||
		  qm->xmax < pm->xmin || qm->ymax < pm->ymin))){
			if(mdebug)fprintf(stderr,"%%adding part %d qpart %d pused %d\n",
				pm-pmoves,qm-qmoves,pm->used);
			if(!pm->used && (qmove == 1)){
			pm->used = 1;
			if(!copied){
				if(pm+1 < &pmoves[pmove]){
					save_el = (pm+1)->moves->prev->next;
					(pm+1)->moves->prev->next = NULL;
				}
				Graphics.path = copypath(pm->moves);
				if(pm+1 < &pmoves[pmove])
					(pm+1)->moves->prev->next = save_el;
				copied++;
				minx = pm->xmin; miny = pm->ymin;
				maxx = pm->xmax; maxy = pm->ymax;
			}
			else if(pmove > 1){	/*kluge*/
				addpart(pm->moves,pm->objct);
				if(pm->xmin < minx)minx = pm->xmin;
				if(pm->ymin < miny)miny = pm->ymin;
				if(pm->xmax > maxx)maxx = pm->xmax;
				if(pm->ymax > maxy)maxy = pm->ymax;
			}
			}
			if(!copied){
				if(qm+1 < &qmoves[qmove]){
					save_el = (qm+1)->moves->prev->next;
					(qm+1)->moves->prev->next = NULL;
				}
				Graphics.path = copypath(qm->moves);
				if(qm+1 < &qmoves[qmove])
					(qm+1)->moves->prev->next = save_el;
				copied++;
				minx = qm->xmin; miny = qm->ymin;
				maxx = qm->xmax; maxy = qm->ymax;
			}
			else {
				addpart(qm->moves, qm->objct);
				if(qm->xmin < minx)minx = qm->xmin;
				if(qm->ymin < miny)miny = qm->ymin;
				if(qm->xmax > maxx)maxx = qm->xmax;
				if(qm->ymax > maxy)maxy = qm->ymax;
			}
			if(qmove == 1)break;
			else continue;
		}
		if(!qin && pin == pct){
			if(!copied){
				if(pm+1 < &pmoves[pmove]){
					save_el = (pm+1)->moves->prev->next;
					(pm+1)->moves->prev->next = NULL;
				}
				Graphics.path = copypath(pm->moves);
				if(pm+1 < &pmoves[pmove])
					(pm+1)->moves->prev->next = save_el;
				copied++;
				minx = pm->xmin; miny = pm->ymin;
				maxx = pm->xmax; maxy = pm->ymax;
			}
			else {
				addpart(pm->moves,pm->objct);
				if(pm->xmin < minx)minx = pm->xmin;
				if(pm->ymin < miny)miny = pm->ymin;
				if(pm->xmax > maxx)maxx = pm->xmax;
				if(pm->ymax > maxy)maxy = pm->ymax;
			}
		}
		if(!pin && qin == qct){
			if(!copied){
				if(qm+1 < &qmoves[qmove]){
					save_el = (qm+1)->moves->prev->next;
					(qm+1)->moves->prev->next = NULL;
				}
				Graphics.path = copypath(qm->moves);
				if(qm+1 < &qmoves[qmove])
					(qm+1)->moves->prev->next = save_el;
				copied++;
				minx = qm->xmin; miny = qm->ymin;
				maxx = qm->xmax; maxy = qm->ymax;
			}
			else {
				addpart(qm->moves, qm->objct);
				if(qm->xmin < minx)minx = qm->xmin;
				if(qm->ymin < miny)miny = qm->ymin;
				if(qm->xmax > maxx)maxx = qm->xmax;
				if(qm->ymax > maxy)maxy = qm->ymax;
			}
			break;
		}
		movect=0;
		k = pm->bpath;
		for(p=pm->moves,ps=pm->sptr; p!= NULL; p=p->next,ps=ps->next){
			if(p->type == PA_MOVETO){
				pint = pinterior[k];
				ps = pst[k++];
				if(movect++ >= pm->objct)break;
				if(mdebug)fprintf(stderr,"%%movect %d pm->objct %d int %d k %d\n",
					movect,pm->objct,pint,k);
				continue;
			}
			if((p->prev->ppoint.x < qm->xmin && p->ppoint.x < qm->xmin) ||
				(p->prev->ppoint.x > qm->xmax && p->ppoint.x > qm->xmax) ||
				(p->prev->ppoint.y < qm->ymin && p->ppoint.y < qm->ymin) ||
				(p->prev->ppoint.y > qm->ymax && p->ppoint.y > qm->ymax))
				continue;
			find_inter(p->prev,p,ps,pint,qm->moves,qm->sptr,qm->bpath,qm->objct);
		}
		if(pts == winters && ((qm != &qmoves[qmove-1]&&pmove>1)||pm->used)){
			continue;
		}
		if(qin || pin != pct){
			reorder();
			find_path(pm, pin, qin);
			pm->used = 1;
				/*removed &&qm->objct > 1*/
			if(inoutline&&pm == &pmoves[pmove-1]&&!qm->used)
				addqhole(qm);
			if(pts != winters)qm->used = 1;
		}
		pts = winters;
	}
	if(!pm->used && inoutline){	/*kluge*/
		addpart(pm->moves,pm->objct);
		if(pm->xmin < minx)minx = pm->xmin;
		if(pm->ymin < miny)miny = pm->ymin;
		if(pm->xmax > maxx)maxx = pm->xmax;
		if(pm->ymax > maxy)maxy = pm->ymax;
	}
	}
	if(mdebug){
		fprintf(stderr,"showpage\n");
		printpath("result",Graphics.path, 1000, 1);
		fprintf(stderr,"showpage\n");
	}
	if(Graphics.path.last != NULL){
		if(!equalpath(clpath,Graphics.path))
			rcode = 1;
	}
	rel_path(clpath.first);
	return(rcode);
}
void
find_inter(struct element *p1, struct element *p2, struct lstats *pp, int pint,
	 struct element *qstart, struct lstats *qstats, int mfir, int mct)
{
	struct element *q, *new;
	struct lstats *qp;
	struct pts pt[MAXINTERS], *xpts=pt;
	double y1, y2, x1, x2, dist, ddist;
	double qy1, qy2, qx1, qx2,xi, yi, d1,xp,yp,xm,ym;
	double dy11, dy12, dy21, dy22, dx11, dx12, dx21, dx22;
	int qin=0, use_ext, incy=0, incx=0, movect=0,i,k=mfir, qint;
	y1 = p1->ppoint.y;
	y2 = p2->ppoint.y;
	if(y1 < y2)incy=1;
	x1 = p1->ppoint.x;
	x2 = p2->ppoint.x;
	if(x1 < x2)incx=1;
	if(pp->type != VERT)d1 = pp->slope*x1 - y1;
	if(mdebug>1)fprintf(stderr,"%%findi p (%f %f) to (%f %f) %d ix %x iy %x\n",
		x1,y1,x2,y2,pp->type,incy,incx);
	for(q=qstart, qp = qstats; q != NULL; q=q->next, qp=qp->next){
		if(q->type == PA_MOVETO){
			qp = qst[k];
			if(qp == 0){
				fprintf(stderr,"Mpanic qp=0 movect %d k %d mct %d\n",movect,k,mct);
				pserror("clip","find_inters");
			}
			qint = qinterior[k++];
			if(movect++ == mct)break;
			continue;
		}
		if(qp == 0){
			fprintf(stderr,"panic qp=0 movect %d k %d mct %d\n",movect,k,mct);
			pserror("clip","find_inters");
		}
		use_ext = 1;
		qx1 = q->prev->ppoint.x;
		qy1 = q->prev->ppoint.y;
		qx2 = q->ppoint.x;
		qy2 = q->ppoint.y;
		dx11 = qx1 - x1; dy11 = qy1 - y1;
		dx12 = qx2 - x1; dy12 = qy2 - y1;
		dx21 = qx1 - x2; dy21 = qy1 - y2;
		dx22 = qx2 - x2; dy22 = qy2 - y2;
		if(pp->type != HORIZ&& qp->type != HORIZ){	/*5/30*/
		if(fabs(dy11) < SMALLDIF){
			if(incy){if(dy12 < 0.)continue;}
			else if(dy12 > 0.)continue;
		}
		if(fabs(dy12) <SMALLDIF){
			if(incy){if(dy11 < 0.)continue;}
			else if(dy11 > 0.)continue;
		}
		if(fabs(dy21) < SMALLDIF){
			if(incy){if(dy22 > 0.)continue;}
			else if(dy22 < 0.){
				continue;}
		}
		if(fabs(dy22) <SMALLDIF){
			if(incy){if(dy21 > 0.)continue;}
			else if(dy21 < 0.)continue;
		} }
		if((qy1 < y1 && qy1 < y2 && qy2 < y1 && qy2 < y2) ||
			(qy1 > y1 && qy1 > y2 && qy2 > y1 && qy2 > y2)){
			continue;
		}
		if(qp->type != VERT && pp->type != VERT){	/*5/29*/
		if(fabs(dx11) < SMALLDIF){
			if(incx){if(dx12 < 0.)continue;}
			else if(dx12 > 0.)continue;
		}
		if(fabs(dx12) <SMALLDIF){
			if(incx){if(dx11 < 0.)continue;}
			else if(dx11 > 0.)continue;
		}
		if(fabs(dx21) < SMALLDIF){
			if(incx){if(dx22 > 0.)continue;}
			else if(dx22 < 0.)continue;
		}
		if(fabs(dx22) <SMALLDIF){
			if(incx){if(dx21 > 0.)continue;}
			else if(dx21 < 0.)continue;
		} }
		if((qx1 < x1 && qx1 < x2 && qx2 < x1 && qx2 < x2) ||
			(qx1 >x1 && qx1 > x2 && qx2 > x1 && qx2 >x2)){
			continue;
		}
		if((fabs(qx1-x1)<SMALLDIF && fabs(qy1-y1)<SMALLDIF) 
			|| (fabs(qx1-x2)<SMALLDIF && fabs(qy1-y2)<SMALLDIF))continue;
		if(pp->type != VERT && qp->type != VERT && fabs(pp->slope - qp->slope)<SMALLDIF){
			continue;
		}
		if(!pp->type && !qp->type){
			xi = (d1 - qp->slope*qx1 + qy1)/(pp->slope-qp->slope);
			yi = qp->slope * (xi - qx1) + qy1;
		}
		else {
			if(pp->type == HORIZ && qp->type == VERT){
				xi = xp = xm = qx1;
				yi = yp = ym = y1;
				use_ext = 0;
			}
			else if(pp->type == VERT && qp->type == HORIZ){
				xi = xp = xm = x1;
				yi = yp = ym = qy1;
				use_ext = 0;
			}
			else if(pp->type== VERT){
				xi = xp = xm = x1;
				yi = yp = ym = qp->slope*(xi-qx1) + qy1;
				use_ext = 0;
			}
			else if(qp->type== VERT){
				xi = xp = xm = qx1;
				yi = yp = ym =pp->slope*(xi-x1) + y1;
				use_ext = 0;
			}
			else if(pp->type== HORIZ){
				yi = yp = ym = y1;
				xi = xp = xm =(yi - qy1)/qp->slope + qx1;
				use_ext = 0;
			}
			else if(qp->type== HORIZ){
				yi = yp = ym = qy1;
				xi = xp = xm =(yi - y1)/pp->slope + x1;
				use_ext = 0;
			}
		}
			/*maybe should check for inoutline here*/
/*		if((fabs(xi-x2)<SMALLDIF && fabs(yi-y2)<SMALLDIF)
			|| (fabs(xi-x1)<SMALLDIF && fabs(yi-y1)<SMALLDIF))continue;*/
		if(use_ext){
			xp = xi + SMALLDIF; xm = xi - SMALLDIF;
			yp = yi + SMALLDIF; ym = yi - SMALLDIF;
		}
		if(mdebug)fprintf(stderr,"%%INT %f %f\n",xi,yi);
		if(qx1 != qx2){
		if(qx2 < qx1){
			if(xp < qx2 || xm > qx1)continue;
		}
		else if(xm > qx2 || xp < qx1)continue;
		}
		if(qy1 != qy2){
		if(qy2 < qy1){
			if(yp < qy2 || ym >qy1)continue;
		}
		else if(ym > qy2 || yp < qy1)
			continue;
		}
		if(x1 != x2){
		if(x2 < x1){
			if(xp < x2 || xm > x1)continue;
		}
		else if(xm > x2 || xp < x1)continue;
		}
		if(y1 != y2){
		if(y2 < y1){
			if(yp < y2 || ym >y1)continue;
		}
		else if(ym > y2 || yp < y1)continue;
		}
		dist=(x1-xi)*(x1-xi)+(y1-yi)*(y1-yi);
		ddist = (qx1-xi)*(qx1-xi)+(qy1-yi)*(qy1-yi);
		if(mdebug)fprintf(stderr,"%%inters %f %f dist %f ddist %f\n",xi,yi,dist,ddist);
		collect(xpts++, xi, yi, qp->prev, pp-1, dist, ddist, q->prev, p1, pint, qint);
		if(xpts >= &pt[MAXINTERS]){
			fprintf(stderr,"too many intersections\n");
			pserror("clip","clip");
		}
	}
	sortq(pt, xpts-pt);
}
int object;
int usepath;
void
find_path(struct smoves *pm, int pin, int qin)
{
	struct pts *pt, *tt;
	struct element *p, *q, *firstq, *savq, *stop;
	struct element *pstart;
	struct lstats *ps, *qs;
	int more, twoleft=0, leftlast=0, i, movect=0;
	int psindex, swcode, justint=0, intleft;
	if(mdebug){
		fprintf(stderr,"[3] 0 setdash\n");
		for(pt=inters, i=1; pt<pts;pt++,i++)
		fprintf(stderr,"%%%d %x inters %f %f q %x (%f %f) qins %d p %x pins %d\n",i,pt,pt->x,pt->y,
			pt->q,pt->q->ppoint.x, pt->q->ppoint.y,pt->qcode,pt->p,pt->pcode);
		for(pt=inters,i=1; pt<pts;pt++,i++)
			fprintf(stderr,"%f sx %f sy moveto (%d)show\n",pt->x,pt->y,i);
		for(pt=inters,i=1; pt<pts;pt++,i++){
			fprintf(stderr,"%f sx 5 sub %f sy moveto (+)show\n",pt->q->ppoint.x,pt->q->ppoint.y);
			fprintf(stderr,"newpath %f sx %f sy moveto",pt->q->ppoint.x,pt->q->ppoint.y);
			if(pt->q->next)fprintf(stderr," %f sx %f sy lineto stroke\n",pt->q->next->ppoint.x,
				pt->q->next->ppoint.y);
			else fprintf(stderr," (S)show\n");
			fprintf(stderr,"%f sx 5 sub %f sy moveto (+)show\n",pt->p->ppoint.x,pt->p->ppoint.y);
			fprintf(stderr,"newpath %f sx %f sy moveto",pt->p->ppoint.x,pt->p->ppoint.y);
			if(pt->p->next)fprintf(stderr," %f sx %f sy lineto stroke\n",pt->p->next->ppoint.x,
				pt->p->next->ppoint.y);
			else fprintf(stderr," (S)show\n");
		}
		fflush(stderr);
	}
	pointtype = PA_MOVETO;
	pstart = pm->moves;
	usepath=2;
	if(!pin && !qin){			/*only intersections*/
		if(pts == inters)return;
		for(p=pstart, pt=inters; p != NULL; ){
		if(mdebug)fprintf(stderr,"%%p %x pt %x pnext %x\n",p,pt->p,p->next);
			if(usepath && p==pt->p){
				pt->p = 0;
				cadd_element(pt->x,pt->y);
				if(usepath==2){
					firstq = pt->q;
					pt++;
					cadd_element(pt->x,pt->y);
					q = pt->q;
					pt->p = 0;
				}
				else q = pt->q;
				usepath=0;
			}
			else if(!usepath){
				for(pt=inters;pt<pts; pt++)
					if(pt->q == q && pt->p != 0)break;
				if(pt == pts){
					closepathOP();
					return;
				}
				cadd_element(pt->x,pt->y);
				p=pt->p;
				pt->p = 0;
				usepath=1;
				pt++;
				continue;
			}
			else p=p->next;
			if(pt->q == firstq){
				closepathOP();
				pointtype = PA_MOVETO;
				usepath = 2;
				for(pt=inters; pt<pts; pt++)
					if(pt->p != 0)break;
				p = pt->p;
				if(pt == pts){
					return;
				}
			}
		}
	}
	else{
		if(mdebug)fprintf(stderr,"%%pin %d qin %d\n",pin,qin);
		q = inters[0].q;
		qs = inters[0].qs;
		object = pm->bpath;
		psindex = pm->bpath;
		ps = pst[psindex++];
		newpt = 0;
		justint = 0;
		intleft = pts-inters;
		for(p=pstart, pt=inters; p != NULL&&movect<pm->objct;){
		if(mdebug)fprintf(stderr,"%%top %d ps %x ins %d member %d p %x t %d obj %d n %d nint %d just %d l %d\n",usepath,
		   ps,ps->inside,ps->member,p,p->type,object,ps->next->inside,pt-inters+1,justint,intleft);	/*4/22*/
			if(ps->inside == 1 &&(!justint||(intleft==1&&!(justint &&!(p->next&&p->next->type == PA_CLOSEPATH&&ps->next->next->inside == 2))))){
				if(object>0 && p->type == PA_MOVETO &&
				 object == ps->member && pinterior[ps->member]){
					pointtype = PA_LINETO;
				}
				else if(usepath != 2)
					pointtype = p->type;
				if(pointtype == PA_MOVETO)closepathOP();
				ps->inside = 2;
				if(p->type == PA_CLOSEPATH){
				  if(mdebug)fprintf(stderr,"%%saw close top %f %f use %d pt->p %x \n",
				  p->ppoint.x,p->ppoint.y,usepath,pt->p);
					closepathOP();
					if(!p->next && !pt->p)return;
					movect++;
					pointtype = PA_MOVETO;
					if(usepath==2){
						p= p->next;
						ps=pst[movect];
						if(ps->inside){
							cadd_element(p->ppoint.x,p->ppoint.y);
							ps->inside=2;
						}
					}
					else if(!pt->p){
						q = firstq;
						movect--;
						usepath = 2;
					}
					else {
						movect--;
						p=pt->p;
						ps=pt->ps;
					}
				}
				else cadd_element(p->ppoint.x,p->ppoint.y);
				object = ps->member;
				/*next outside & have wrong inters*/
				if(usepath==2 && ps->next->inside==0 && p != pt->p){
					for(tt=inters; tt<pts;tt++)
						if(tt->p == p)break;
					if(tt == pts){
					if(mdebug)fprintf(stderr,"%%Big problems pt %x\n",pt);
					}
					else pt=tt;
				}
			}
			else if(usepath == 1 && p != pt->p){
				if(mdebug)fprintf(stderr,"%%outside %x pt %x %d\n",p,pt->p,pt-inters+1);
				for(tt=inters; tt<pts; tt++)
					if(tt->p == p->prev)break;
				if(tt != pts){
					pt = tt;
					p = pt->p;
					ps = pt->ps;
				}
			}
			if(usepath && p == pt->p){
			    if(mdebug)fprintf(stderr,"%%adding inters %d grp %d pc %d qc %d\n",pt-inters+1,pt->group,pt->pcode,pt->qcode);
			    cadd_element(pt->x,pt->y);
			    intleft--;
			    pt->p = 0;
			    justint=0;
			    swcode = pt->qcode+pt->pcode*4;
			    if(mdebug)fprintf(stderr,"%%switching on %d\n",
				swcode);
			    switch(swcode){
			    default: fprintf(stderr,"CASE not in switch %d q %d p %d\n",
				    pt->qcode+pt->pcode*4,pt->qcode,pt->pcode);
				break;
			    case NONE:
				if(usepath==2){
					firstq = pt->q;
					if(mdebug)fprintf(stderr,"%%none new firstq %x\n",firstq);
					pt++;
					if(pt < pts){
					cadd_element(pt->x,pt->y);
					intleft--;
					if(mdebug)fprintf(stderr,"%%added inters %d pc %d qc %d\n",pt-inters+1,pt->pcode,pt->qcode);
					q = pt->q;
					qs = pt->qs;
					pt->p = 0;
					if(pt->qcode == RIGHT){
						q = q->next;
						qs = pt->qs->next;
						q=addqpath(q,qs,(pt+1)->q, 0,1);
						if(mdebug)fprintf(stderr,"%%q returned NONE %x\n",q);
					}
					else if(pt->qcode == LEFT){	/*G*/
						if(q->prev != NULL){
							q = q->prev;
							qs=pt->qs-1;
						}
						leftlast=1;
					}
					else if(checkint(q)){ /*5/15*/
							pt=newpt;
							p=pt->p;
							ps=pt->ps;
							usepath=1;
							continue;
						}
					}
				}
				else{ q = pt->q; qs = pt->qs;
					if(p == (pt+1)->p){/*5/9*/
						pt++;
						continue;
					}
				}
				if(mdebug)fprintf(stderr,"%%none %x\n",q);
				usepath = 0;
				break;
			    case 8:	/*p right q outside - moved*/
			    case 9:	/*p right q left*/
			    case 13:	/*p both q left*/
			    case 11:	/*p right q both 4/17*/
			    case 12:	/*p both q none*/
				if(usepath==2)firstq = pt->q;
				pt++;
				if(pt >= pts||!pt->p){	/*4/18*/
					p=p->next;
					ps = ps->next;
					p=addqpath(p, ps, 0, 0, 0);
					ps=saveit;
					if(p->next && !ps->next->inside||(pt==pts && (pt-1)->group > 1)){
						for(tt=inters; tt<pts; tt++)
							if(tt->p == p)break;
						if(tt->p == p){
							pt=tt;
							continue;
						}
					}
					usepath = 0;	/*4/16*/
					if(mdebug)fprintf(stderr,"%%8 last inters p ins %d p %x\n",ps->inside,p);
					q=firstq;
					goto checkq;
					/*goto ckinters;*/
				}
				if(p != pt->p){
					p = p->next;
					ps=ps->next;
					if(mdebug)fprintf(stderr,"%%p %x ps %x inside %x\n",p,ps, ps->inside);
					p = addqpath(p, ps, pt->p, 0, 0);
					ps = saveit;
					if(mdebug)fprintf(stderr,"%%returned %x %x type %d pt %d ptp %x\n",p,ps,p->type,pts-pt,pt->p);
					if(p != pt->p && p != 0 && pt->p != 0){
						if(mdebug)fprintf(stderr,"%%looking for %x\n",p);
						for(tt=inters; tt < pts; tt++)
							if(tt->p == p)break;
						if(tt->p == p){
	if(mdebug)fprintf(stderr,"%%case 11 adding inters %d pc %d qc %d\n",tt-inters+1,tt->pcode,tt->qcode);
							cadd_element(tt->x,tt->y);
							intleft--;
							tt->p = 0;
							pt = tt;
							q = pt->q;
							qs = pt->qs;
							if(pt->qcode >= RIGHT){
								q=addqpath(q->next, qs->next, 0, 0, 1);
								qs = saveit;
	if(mdebug)fprintf(stderr,"%%q %x firstq %x\n",q,firstq);
								newpt = 0;
							}
							usepath = 0;
			/*5/21*/			if(q == firstq){
								closepathOP();
								pointtype=PA_MOVETO;
							}
							/*break;*/
							goto checkq;
						}
						else { usepath=0;
							goto checkq;
						}
					}
				}
				q = pt->q;
				qs = pt->qs;
				usepath=1;
				break;
			    case LEFT:	/*p none q left*/
			    case 4:	/*p left q none*/
			    case 7:	/*p left q both*/
			    case 5:	/*p left q left*/
				i=0;
				if(usepath==2){
					firstq = pt->q;
					if(mdebug)fprintf(stderr,"%%new firstq %x\n",firstq);
					if(!(pt-inters))i=1;
				}
				else if(pt->q == firstq && swcode != 7){
					q = pt->q;
					qs = pt->qs;
					break;
				}
				if(pt->group > 2 && pt->elt%2 && p == (pt+1)->p){
					pt++;
					justint=1;
					break;
				}
				if(pt->qcode >= RIGHT){
					q = pt->q->next;
					qs = pt->qs->next;
					q = addqpath(q, qs, 0, 0, 1);
					qs = saveit;
				}
				if(newpt){
					pt = newpt;
					p = pt->p;
					ps = pt->ps;
					justint=1;
					newpt=0;
				}
				else {
					if(p == (pt+1)->p && ps->inside != 2)pt++;/*5/9,15*/
					else if(checkint(q)){
						pt=newpt;
						newpt=0;
						justint=1;
					}
					else if(checkint(q->next)){
						pt=newpt;
						newpt=0;
						justint=1;
					}
					else pt++;
				}
				if(pt >= pts|| !pt->p)break;
				p = pt->p;
				ps = pt->ps;
				q = pt->q;
				qs = pt->qs;
				if(mdebug)fprintf(stderr,"%%q left %x pt %d p %x pc %d qc %d\n",q, pt-inters+1,p,pt->pcode,pt->qcode);
				if(i && q == firstq && !pt->qcode){q=q->next;qs=qs->next;}/*5/15*/
				usepath = 1;
				break;
			    case RIGHT:	/*p none q right*/
			    case BOTH:	/*p none q both*/
			    case 6:	/*p left q right*/
			    case 10:	/*p right q right*/
			    case 14:	/*p both q right*/
			    case 15:	/*p both q both*/
				if(usepath == 2){
					firstq = pt->q->next;
					stop=0;
				if(mdebug)fprintf(stderr,"%%c15 firstq %x qty %d qind %d\n",
					firstq,pt->q->type,pt->qs->inside);
				}
				else stop = firstq;
				if(swcode >= 10 && ps->next->inside == 1){
					for(tt=inters; tt<pts; tt++)
						if(tt->p == p)break;
					if(mdebug)fprintf(stderr,"%%c15 tt %d pt %d\n",tt-inters,pt-inters);
					if(tt >= pts || tt < pt){
						stop = (pt+1)->p;
						p=addqpath(p->next, ps->next, stop,0,0);
						ps = saveit;
						if(p != stop)usepath=0;
						else pt++;
						continue;
					}
				}
				q = pt->q->next;
				qs = pt->qs->next;
				pt++;
				if(twoleft){
					twoleft = 0;
					usepath = 1;
					break;
				}
	/*5/9*/			q = savq = addqpath(q,qs,stop,0,1);
				qs = saveit;
				if(mdebug)fprintf(stderr,"%%c15 q return %x usepath %d ins %d ptq %x stop %x\n",
					q,usepath,qs->next->inside,pt->q,stop);
	/*5/9*/			if(q == stop && !newpt){
	if(mdebug)fprintf(stderr,"%%saw stop closing\n");
	closepathOP();pointtype=PA_MOVETO;usepath=2;justint=1; goto ckinters;}
				if((!qs->next->inside && q != pt->q)||newpt){/*4/17*/
					if(newpt){
						pt = newpt;
						p = pt->p;
						ps = pt->ps;
						usepath=1;
						newpt = 0;
						justint=1;
						/*break;*/
						continue;	/*5/28*/
					}
					for(tt=inters; tt<pts; tt++)
						if(q == tt->q && tt->p){
							pt=tt;
							p = pt->p;
							ps = pt->ps;
							usepath=2;
							break;
						}
					if(tt == pt && pt != pts)break;
				}
				if(pt >= pts){
					if(mdebug)fprintf(stderr,"%%overran3\n");
					if(q->next == firstq)q=q->next;
					usepath=0;
					goto ckinters;
				}
				if(usepath == 2){usepath=1;continue;}
				else usepath =0;			/*X*/
			    }
			}
			else if(!usepath){
checkq:
			    if(mdebug)fprintf(stderr,"%%usepath=0 searching for %x\n",q);
			    if(checkint(q)){	/*5/15*/
				pt=newpt;
				newpt=0;
			    }
			    else {
			    for(pt=inters, more=0; pt<pts; pt++){
				if(pt->p != 0)more++;
				if(pt->q == q && pt->p != 0)break;
			    }
			    }
			    if(pt >= pts){
				if(!more){
					if(mdebug)fprintf(stderr,"%%not usepath quitting q %x ps ins %d p %x ps %x movect %d obj %d\n",q,ps->inside,p,ps,movect,pm->objct);
					if(ps->inside==1)
						addqpath(p, ps, 0, 0, 0);
					else if(ps->next->inside==1)
						addqpath(p->next,ps->next,0,0, 0);
					closepathOP();
					if(movect < pm->objct)
						goto getrest;
					return;
				}
				if(mdebug)fprintf(stderr,"%%didn't find q %x\n",q);
					/*the following is wrong - order problem*/
				if(leftlast){
					q = q->next;
					qs=qs->next;
					if(mdebug)fprintf(stderr,"%%calling addq with 0 q %x\n",q);
					q = addqpath(q, qs, 0, 1, 1);
					qs = saveit;
					leftlast=0;
					newpt=0;
				}
				for(pt=inters; pt<pts; pt++)
					if(pt->p)break;
				if(pt >= pts){
					closepathOP();
					return;
				}
				closepathOP();	/*NEW*/
				pointtype=PA_MOVETO;
			    }
			    if(pointtype == PA_MOVETO)savq=pt->q;
			    else savq=0;
			    if(mdebug)fprintf(stderr,"%%qcode %d pcode %d adding inters %d q %x fq %x\n",
				pt->qcode, pt->pcode,pt-inters+1,q,firstq);
			    if(pt->pcode == LEFT && pointtype==PA_MOVETO)
				justint=1;	/*5/15*/
			    cadd_element(pt->x,pt->y);
			    intleft--;
			    twoleft=0;
			    p = pt->p;
			    ps = pt->ps;
			    pt->p = 0;
			    usepath = 1;
			    if(pt->pcode == BOTH && q != firstq){
				for(tt=inters; tt<pts; tt++)
					if(tt->p == p){
						p = p->next;
						ps = ps->next;
						break;
					}
			    }
			    if(pt->pcode >= RIGHT && p->next && ps->next->inside != 2/*&& q == firstq*/){
				stop=(pt+1)->p;
				p=addqpath(p->next, ps->next, stop, 0, 0);
				ps=saveit;
				if(p == stop){
					pt++;
					continue;
				}
/*4/23*/		        if(!(ps->next->inside)){
				    for(tt=inters; tt<pts; tt++)
					if(tt->p == p){
						newpt=tt;
						break;
					}
				}
			    }
			    if(pt->qcode >= RIGHT){
				q = addqpath(pt->q->next, pt->qs->next, 0, 0, 1);
				qs = saveit;
			    }
			    if(newpt){
				pt = newpt;
				newpt = 0;
				p=pt->p;
				ps = pt->ps;
				justint=1;
			    }
			    else{
				 pt++;
				if(justint){	/*5/15*/
					p=pt->p;
					ps=pt->ps;
				}
			    }
			    if(!pt->p || pt>=pts)
				for(pt=inters; pt<pts; pt++)
					if(pt->p)break;
			    if(pt >= pts){
				if(mdebug){fprintf(stderr,"%%overran 4\n");
					fprintf(stderr,"%%qcode %d\n",(pt-1)->qcode);}
				if((pt-1)->qcode == RIGHT|| (pt-1)->qcode == BOTH){
					q = (pt-1)->q->next;
					qs = (pt-1)->qs->next;
					q = addqpath(q, qs, 0, 0, 1);
					if(newpt){
						pt=newpt;
						ps = pt->ps;
						p = pt->p;
						usepath=1;
						newpt=0;
						justint=1;
					}
					else{
						closepathOP();
						goto endgame;
					}
				}
			    }
/*	5/9		    else if(pt->qcode==LEFT&&(pt-1)->qcode==LEFT&&pt->pcode==NONE){
				if(mdebug)fprintf(stderr,"%%found 2 left after search adding inters %d pc %d qc %d\n",pt-inters+1,pt->pcode,pt->qcode);
				cadd_element(pt->x,pt->y);
				intleft--;
				q = pt->q;	/*more order problems 6 nD
				qs = pt->qs;
				q = addqpath(q,qs,0,1, 1);
				qs = saveit;
				twoleft = 1;
				usepath = 0;
				newpt=0;
			    }*/
/*			    else if((pt-1)->q == pt->q && pt->p != 0){
				if(mdebug)fprintf(stderr,"%%q's equal resetting p %x just %d\n",justint);
				p = pt->p;
				ps = pt->ps;
			    }*/
			    if(savq){firstq=savq;q=pt->q;qs=pt->qs;continue;}	/*2479 4/24*/
			}
			else {
				if(p->type == PA_CLOSEPATH){
					movect++;
					ps = pst[psindex++];
					if(q == 0)closepathOP();/*bug?*/
				}
				else ps = ps->next;
				p=p->next;
				if(p == 0)goto ckinters;
			}
			if(mdebug)fprintf(stderr,"%%q %x firstq %x ptq %x p %x usepath %d\n",q,firstq,pt->p,p,usepath);
			if(q == firstq){
			    if(pt->p){
				if(mdebug)fprintf(stderr,"%%last pt ptq %x q %x qp %x\n",pt->q,q,q->prev);
				if(pt->q == q || pt->q == q->prev){
					cadd_element(pt->x,pt->y);
					intleft--;
					p = pt->p->next;
					ps = pt->ps->next;
	if(mdebug)fprintf(stderr,"%%p %f %f pi %d qi %d added inters %d pc %d qc %d\n",
	p->ppoint.x,p->ppoint.y,ps->inside,qs->inside,pt-inters+1,pt->pcode,pt->qcode);
					pt->p = 0;
					if(ps->inside==1){
						p=addqpath(p, ps, (pt+1)->p,0, 0);
						ps=saveit;
	if(mdebug)fprintf(stderr,"%%returned p %x %d ps %x n %d nn %d\n",p,p->type,ps,ps->next->inside,ps->next->next->inside);
						if(!ps->next->inside){
						  for(tt=inters; tt<pts; tt++)
							if(tt->p == p)break;
						  if(tt != pts){
							pt=tt;
							p=pt->p;
							ps=pt->ps;
							q=pt->q;
							qs=pt->qs;
							continue;
						  }
						}
					}
					if(pt->qcode >= RIGHT){
						addqpath(pt->q->next,pt->qs->next, 0, 0, 1);
						if(newpt){
							pt=newpt;
							p=pt->p;
							ps=pt->ps;
							q=pt->q;
							qs=pt->qs;
							newpt = 0;
							justint=1;
							continue;
						}
					}
				}
			    }
ckinters:		    for(pt=inters; pt<pts; pt++)
				if(pt->p)break;
			    if(pt == pts){
endgame:			if(!p){ closepathOP(); return;}
	if(mdebug)fprintf(stderr,"%%checking endgame %d obj %d %x pn %x ps %x mv %d pind %d\n",
				  usepath,pm->objct,p,p->next,ps,movect,psindex);
				if(usepath != 2 || movect < pm->objct){
				if(movect<pm->objct && p->type != PA_MOVETO &&
				  ps->inside != 1){ /*4/16*/
					if(p->type == PA_CLOSEPATH){
						psindex = ps->member+1;
						ps = pst[psindex];
					}
					else ps=ps->next;
					p=p->next;
				}
				if(p){
getrest:				  if(mdebug)fprintf(stderr,"%%getting rest of p %f %f p %x t %d i %d movect %d pindex %d member %d\n",
				     p->ppoint.x,p->ppoint.y,p,p->type,ps->inside,movect, psindex,ps->member);
					if(psindex <= ps->member)psindex=ps->member+1;
					for(;movect < pm->objct && p != NULL;p=p->next){
						if(ps->inside==1){
							if(p->type != PA_CLOSEPATH){
							pointtype = p->type;
							if(pointtype == PA_MOVETO)
								closepathOP();
							cadd_element(p->ppoint.x,p->ppoint.y);
							}
							else closepathOP();
							
						}
						if(p->type == PA_CLOSEPATH){
						movect++;
						ps = pst[psindex++];
						}
						else ps = ps->next;
						
					}
					
				}
				closepathOP();
				return;
				}
			    }
			    if(pt->q != q /*&& (pt-1)->q != q*/){
	if(mdebug)fprintf(stderr,"%%closing justint %d\n",justint);
			    	closepathOP();
			    	pointtype = PA_MOVETO;
			    	usepath = 2;
			    }
			    p = pt->p;
			    ps = pt->ps;
			    q = pt->q;
			    qs = pt->qs;
			}
		}
	}
	if(mdebug)fprintf(stderr,"%%fell out p %x movect %d\n",p,movect);
	closepathOP();
}
struct element *
addqpath(struct element *q, struct lstats *qs, struct element *stop, int reverse,int isq)
{
	struct element *pastq, *firstq;
	struct lstats *pastqs;
	int ct=0, usedalt=0, stoploop=0;
	newpt = 0;
	saveit = pastqs = qs;
	firstq = pastq = q;
	if(mdebug)fprintf(stderr,"%%addq called with q %x %x stop %x reverse %d ins %d isq %d\n",q,qs,stop,reverse,qs->inside,isq);
	while(qs->inside==1){
		object = qs->member;
		if(cadd_element(q->ppoint.x,q->ppoint.y)){
			ct++;
			pastq = q;
			pastqs = qs;
		}
		else if(q->type == PA_MOVETO)
			pastqs = qs;
		qs->inside = 2;
		if(isq /*&& inoutline*/ && checkint(q))break;
		if(q == stop)break;
		if(reverse){
			q = q->prev;
			qs--;
			saveit = qs;
		}
		else {
			if(qs->altq){q=qs->altq; usedalt=1;}
			else if(q->type == PA_CLOSEPATH){
				if(mdebug)fprintf(stderr,"%%saw close in addq\n");
				if(stoploop)break;
				stoploop++;
				while(q=q->prev)
					if(q->type == PA_MOVETO){
						if(qs->next->inside == 2){
							closepathOP();
							usepath=2;
							pointtype = PA_MOVETO;
						}
						else pastq=q;
						break;
					}
			}
			else q = q->next;
			qs= qs->next;
			saveit = qs;
			if(q == 0)break;
			if(mdebug)fprintf(stderr,"%%newq %x qs %x inside %d memb %d type %d obj %d\n",q,qs,qs->inside,qs->member,q->type,object);
		}
	}
	if(mdebug)fprintf(stderr,"%%stop %x pastq %x q %x ct %x alt %d\n",stop,pastq, q, ct, usedalt);
	if(newpt)return(q);
	if(usedalt && ct == 1)return(firstq);
	if(q != stop || !q){
		saveit = pastqs;
		return(pastq);
	}
	if(!reverse && (q != stop|| /*q->prev == firstq*/ct==1|| !q)){
		saveit = pastqs;
		return(pastq);	/*for 4 & T*/
	}
	return(q);
}
int
checkint(struct element *q){
	struct pts *pt;
	double smdist=1.e24;
	newpt=0;
	for(pt=inters; pt<pts; pt++){
		if(pt->q == q && pt->p){
			if(pt->qdist < smdist){
				smdist = pt->qdist;
				newpt = pt;
			}
		}
	}
	if(mdebug)if(newpt)fprintf(stderr,"%%setting newpt %x %d\n",newpt,newpt-inters+1);
	if(!newpt)return(0);
	else return(1);
}
void
sortq(struct pts pt[], int n)
{
	int i;
	struct pts *xpts;
	if(n == 0)return;
	if(n > 1){
		qsort(pt, n, sizeof(*pt), qcmp);
		if(mdebug)fprintf(stderr,"%%qsort %d pts\n",n);
	}
	xpts = pt;
	for(i=0;i<n;i++){
		pts->q = xpts->q;
		pts->p = xpts->p;
		pts->qs = xpts->qs;
		pts->ps = xpts->ps;
		pts->x = xpts->x;
		pts->y = xpts->y;
		pts->qcode = xpts->qcode;
		pts->pcode = xpts->pcode;
		pts->qdist = xpts->qdist;
		pts->group = n;
		pts->elt = i;
		pts++->dist = xpts++->dist;
		if(pts >= &winters[MAXPATHS-1]){
			fprintf(stderr,"too many points\n");
			pserror("clip","clip");
		}
	}
}
int
qcmp(struct pts *q1, struct pts *q2)
{
	double d;
	d = q1->dist - q2->dist;
	if(d < 0.)return(-1);
	if(d == 0.)return(0);
	else return(1);
}

void
collect(struct pts *xpts, double x, double y,struct lstats *qp,
	struct lstats *pp, double dist, double qdist, struct element *q,
	struct element *p, int pint, int qint)
{
	struct element *nq;
	xpts->q = q;
	xpts->p = p;
	xpts->qs = qp;
	xpts->ps = pp;
	xpts->x=x;
	xpts->y=y;
	xpts->dist=dist;
	xpts->qdist = qdist;
	xpts->pint = pint;
	xpts->qint = qint;
	if(mdebug)fprintf(stderr,"%% %f %f qp %x %d %d  %d dist %f %f\n",x,y,qp,
		qp->inside,qp->prev->inside,qp->next->inside,dist,qdist);
	if(qp->inside == 0){
		if((qp->next)->inside==0)xpts->qcode = NONE;
		else{ xpts->qcode = RIGHT;
			if((q->next)->type == PA_CLOSEPATH){
			if(mdebug)fprintf(stderr,"%%found closepath\n");
				nq = q;
				while(nq=nq->prev){
					if(nq->type == PA_MOVETO){
						(qp->next)->altq = nq;
						if(mdebug)fprintf(stderr,"%%setting altq to %x \n",nq);
						break;
					}
				}
			}
		}
	}
	else if((qp->next)->inside == 0){
		xpts->qcode = LEFT;
		if(q->type == PA_MOVETO)
			xpts->q = find_close(q);
	}
	else xpts->qcode = BOTH;
	if(pp->inside == 0){
		if((pp+1)->inside==0)xpts->pcode = NONE;
		else xpts->pcode = RIGHT;
	}
	else if((pp+1)->inside == 0)xpts->pcode = LEFT;
	else xpts->pcode = BOTH;
}
struct element *
find_close(struct element *q)
{
	if(mdebug)fprintf(stderr,"%%using close not moveto %x\n",q);
	while(q){
		if(q->type == PA_CLOSEPATH)
			return(q);
		q = q->next;
	}
}
struct element *
find_open(struct element *q)
{
	if(mdebug)fprintf(stderr,"%%using open not close\n");
	while(q=q->prev){
		if(q->type == PA_MOVETO)
			return(q);
	}
}

int
cadd_element(double x1, double y1)
{
	struct element *p;
	static struct element *pfirst;
	int movect=0;
	if(pointtype != PA_MOVETO){
		for(p=pfirst; p != NULL; p=p->next){
			if(fabs(p->ppoint.x-x1)<SMALLDIF&&fabs(p->ppoint.y-y1)<SMALLDIF)
				return(0);
		}
	}
	if(mdebug)fprintf(stderr,"%%	adding (%f %f) %d\n",x1,y1,pointtype);
	copied = 1;
	add_element(pointtype, makepoint(x1,y1));
	if(x1 > maxx)maxx = x1;
	if(x1 < minx)minx = x1;
	if(y1 > maxy)maxy = y1;
	if(y1 < miny)miny = y1;
	if(pointtype == PA_MOVETO){
		pfirst = Graphics.path.last;
		pointtype = PA_LINETO;
	}
	return(1);
}
struct element *
rev(struct working *mm, struct working *mx, struct path *path)
{
	struct element *element, *next, *save, *first;
	if(mdebug)fprintf(stderr,"%%reversing %d\n",mm-mx);
	first = mm->sm->moves;
	first->type = PA_CLOSEPATH;
	element = first->next;
	while(1){
		if(element->type == PA_CLOSEPATH){
			element->type = PA_MOVETO;
			next = element->next;
			element->next = element->prev;
			element->prev = first->prev;
			if(first->prev != 0)first->prev->next = element;
			first->prev = first->next;
			first->next = next;
			if(first->next != 0)first->next->prev = first;
			if(path->first == first)
				path->first = element;
			if(path->last = element)
				path->last = first;
			mm->close = first;
			mm->sm->moves = element;
			recomp(element, mm->sm->sptr);
			return(element);
		}
		next = element->next;
		element->next = element->prev;
		element->prev = next;
		element = next;
	}
}
int
inside(struct pspoint pt, struct element *pstart, struct lstats *pp, 
	struct lstats *pst[], int qct, int startq, int interior[])
{
	struct element *p;
	struct ins xii[MAXINTERS], yii[MAXINTERS];
	double  temp, x,y,samex=0.,samey=0.;
	int j=0, k=0, ik, ij, movect=0, iflag=0, ii, holes=0;
	x = pt.x;
	y = pt.y;
	for(p=pstart; p != NULL; p=p->next, pp=pp->next){
		if(p->type == PA_MOVETO){
			if(movect++ == qct)break;
			if(!pp||qct>1){
				iflag = interior[startq];
				if(iflag)holes=1;
				pp = pst[startq++];
			}
			continue;
		}
		if((fabs(x-p->prev->ppoint.x)<SMALLDIF &&
			 fabs(y-p->prev->ppoint.y)<SMALLDIF)||
			(fabs(x - p->ppoint.x)<SMALLDIF &&
			 fabs(y - p->ppoint.y)<SMALLDIF))
				goto isinside;
		if((x <= p->prev->ppoint.x && x >= p->ppoint.x) ||
			(x <= p->ppoint.x && x >= p->prev->ppoint.x)){
			switch(pp->type){
			case VERT: temp = y; break;
			case HORIZ: temp = p->ppoint.y; break;
			default: temp = pp->slope*(x - p->ppoint.x)+p->ppoint.y;
			}
			yii[j].flag = iflag;
			if(j == 0)yii[j++].value = temp;
			else {
				for(ii=0;ii<j;ii++)
					if(fabs(yii[ii].value-temp)<SMALLDIF)break;
				if(ii == j)yii[j++].value = temp;
				else {
					samey=temp;
				}
			}
			if(j >= MAXINTERS){
				fprintf(stderr,"inside too many pts\n");
				pserror("clip","inside");
			}
		}
		if((y <= p->prev->ppoint.y && y >= p->ppoint.y) ||
			(y <= p->ppoint.y && y >= p->prev->ppoint.y)){
			switch(pp->type){
			case VERT: temp = p->ppoint.x; break;
			case HORIZ: temp = x; break;
			default: temp = (pp->slope*p->ppoint.x+(y-p->ppoint.y))/pp->slope;
			}
			xii[k].flag = iflag;
			if(k == 0)xii[k++].value = temp;
			else {
				for(ii=0;ii<k;ii++)
					if(fabs(xii[ii].value-temp) < SMALLDIF)break;
				if(ii == k)xii[k++].value = temp;
				else if(p->type == PA_LINETO){
					samex = temp;
				}
			}
		}
	}
	if(k == 0 || j == 0)goto outside;
	qsort(xii, k, sizeof(*xii), icmp);
	qsort(yii, j, sizeof(*yii), icmp);
	if(mdebug>1){
	fprintf(stderr,"%%k %d j %d (%f %f) same %f %f\n%%x ",k,j,x,y,samex,samey);
	for(ik=0;ik<k;ik++)fprintf(stderr," %f ",xii[ik].value);
	fprintf(stderr,"\n%%y ");
	for(ik=0;ik<j;ik++)fprintf(stderr," %f ",yii[ik].value);
	fprintf(stderr,"\n");fflush(stderr);}
	if(k%2){
		for(ik=0;ik<k;ik++)
			if(fabs(x - xii[ik].value) < SMALLDIF)break;
		if(ik == k && samex == 0.)
			goto outside;
	}
	if(k%2 && xii[0].value == samex && k > 1)ik=1;
	else ik=0;
	if(x < xii[ik].value){
		for(;ik<k;ik++)
			if(x >= xii[ik].value)break;
		if(ik == k)goto outside;
		if(ik > 0 && xii[ik-1].flag && xii[ik].flag)goto outside;
	}
	else{ for(;ik<k;ik++)
		if(x <= xii[ik].value)break;
		if(ik == k)goto outside;
		if(ik > 0 && xii[ik-1].flag && xii[ik].flag)goto outside;
		if(samex != 0 && x > samex)ik++;
	}
	if(j%2){
		for(ij=0;ij<j;ij++)
			if(fabs(y - yii[ij].value) < SMALLDIF)break;
		if(ij == j && samey == 0.)
			goto outside;
	}
	if(j%2 && yii[0].value == samey && j > 1)ij=1;
	else ij=0;
	if(y < yii[ij].value){
		for(;ij<j;ij++)
			if(fabs(y - yii[ij].value) < SMALLDIF)break;
		if(ij == j)goto outside;
		if(ij > 0 && yii[ij-1].flag && yii[ij].flag)goto outside;
	}
	else {
		for(;ij<j;ij++){	/*7/16 added fabs*/
			if(y < yii[ij].value&& !(fabs(y-yii[ij].value)<SMALLDIF)){
				if(mdebug>1)fprintf(stderr,"%%%f < %f\n",y,yii[ij].value);
				break;	/*9/14 =*/
			}
			if(pst==0 && y == yii[ij].value){
if(mdebug>1)fprintf(stderr,"EQUAL %d\n",ij);
				/*break;*/	/*6/30/92*/
				goto isinside;
			}
		}
		if(ij == j)goto outside;
		if(ij > 0 && yii[ij-1].flag && yii[ij].flag)goto outside;
	}
	if(mdebug>1)fprintf(stderr,"%%ij %d ik %d qct %d ",ij,ik,qct);
	if(j>2 && !(j%2) && !(ij%2) && (qct==1||!holes))goto outside;
	if(k>2 && !(k%2) && !(ik%2) && (qct==1||!holes))goto outside;
	if(!(ik%2) && !(ij%2))goto outside;
isinside:
	if(mdebug>1)fprintf(stderr,"%%ret 1\n");
	return(1);
outside:
	if(mdebug>1)fprintf(stderr,"%%ret 0\n");
	return(0);
}

int
icmp(struct ins *w1, struct ins *w2)
{
	if(w1->value < w2->value)return(-1);
	if(w1->value > w2->value)return(1);
	if(w1->value == w2->value)return(0);
}

int
getslope(struct path *path, struct lstats *pstats, struct smoves moves[],
	int interior[], struct lstats *start[], struct element *estart[])
{
	struct element *p, *pp, *first, *newf;
	struct element *mmcln, *mxopp, *mxcln;
	struct lstats *scln, *sxopp, *sxcln;
	int movect = -1, ii=0, flag = -1, mct;
	double cw=0., ccw=0., d1, d2, dist;
	double lcw=0., lccw=0.;
	double dx1,dy1,dx2,dy2;
	struct lstats *ps=pstats, *savps;
	struct smoves *ms;
	struct working m[MAXMOVE], *mp=m, *mpm, *mm, *mx, *savmpm, *lastm;
	double deltax, rminx, rminy, rmaxx, rmaxy;
	int move=0, movem, i, j, usex=0, usey=0, k, hole, count=0, havehole=0;
	int direction[MAXMOVE];
	mp->sm = moves;
	ps->prev = 0;
	for(p=path->first; p != NULL; p=p->next, ps++){
		if(ps >= pstats+MAXPATHS)
			pserror("implementation limit - increase MAXPATHS", "clip");
		ps->altq = 0;
		if(p->type == PA_MOVETO){
			mp->sm->moves = p;
			mp->sm->sptr = ps;
			ps->slope = -999999.;
			savps = ps;
			ps->member = move;
			mp->sm->xmin = mp->sm->xmax = p->ppoint.x;
			mp->sm->ymin = mp->sm->ymax = p->ppoint.y;
			mp->sm->objct = 1;
			mp->sm->bpath = move;
			mp->sm->used = 0;
			mp->used = 0;
			mp->sm->count = 1;
			mp->index = move;
			mp->hashole = mp->hole = 0;
			mpm=mp++;
			mp->sm = mpm->sm + 1;
			if(move++ >= MAXMOVE)
				pserror("implementation limit - increase MAXMOVE", "clip");
			continue;
		}
		mpm->sm->count++;
		(ps-1)->next = ps;
		ps->prev = ps-1;
		if(p->type == PA_CLOSEPATH){
			ps->next = savps;
			savps->prev = ps;
			mpm->close = p;
			mpm->sclose = ps;
		}
		ps->member = move-1;
		ps->type = 0;
		deltax = p->ppoint.x - p->prev->ppoint.x;
		if(fabs(deltax) >= SMALLDIF){
			ps->slope = (p->ppoint.y - p->prev->ppoint.y)/deltax;
			if(fabs(ps->slope) < SMALLDIF)ps->type = HORIZ;
		} else{
			ps->type = VERT;
			ps->slope = 1000.;
		}
		if(p->ppoint.x < mpm->sm->xmin)mpm->sm->xmin = p->ppoint.x;
		if(p->ppoint.x > mpm->sm->xmax)mpm->sm->xmax = p->ppoint.x;
		if(p->ppoint.y < mpm->sm->ymin)mpm->sm->ymin = p->ppoint.y;
		if(p->ppoint.y > mpm->sm->ymax)mpm->sm->ymax = p->ppoint.y;
	}
	if(mdebug){
	    for(j=0, mm=m;j<move; j++, mm++)
		fprintf(stderr,"%%	x %f %f y %f %f elem %x ct %d\n",
			mm->sm->xmin, mm->sm->xmax, mm->sm->ymin, mm->sm->ymax,
			mm->sm->moves,mm->sm->count);}
	k=move;
	if(move == 1)
		interior[0] = 0;
	ii=hole=0;
	mm=m;
	mm->obj = 0;
	for(p=path->first; p != NULL; p=p->next){
		if(p->type == PA_MOVETO)continue;
		if(p->type == PA_CLOSEPATH){
			if(mdebug)fprintf(stderr,"%%cw %f ccw %f (%f %f)\n",cw,ccw,p->ppoint.x,p->ppoint.y);
			if(cw > ccw)direction[ii] = CW;
			else if(ccw > cw)direction[ii] = CCW;
			else{ fprintf(stderr,"%%unknown direction %f %f\n",cw, ccw);fflush(stderr);}
			cw = ccw = 0.;
			if(mm->obj != NULL){
				if(mdebug)fprintf(stderr,"%%%d hole in %d ct %d %d\n",
					mm-m,mm->obj-m,mm->hole,hole);
				if(mm->hole != hole){
					interior[ii] = 0;
					mm->obj = 0;
				}
				else{ interior[ii] = 1;
					havehole++;
					m[mm->obj-m].hashole++;
				}
			}
			else interior[ii] = 0;
			ii++;
			mm++;
			hole=0;
			mm->obj = 0;
			continue;
		}
		if(fabs(p->ppoint.y-mm->sm->ymin)<SMALLDIF&&p->next != NULL&&p->prev != NULL){
			if(p->ppoint.x <= p->next->ppoint.x &&
				p->prev->ppoint.x <= p->ppoint.x)ccw++;
			else if(p->ppoint.x >= p->next->ppoint.x &&
				p->prev->ppoint.x >= p->ppoint.x)cw++;
			if(move>1)checkit(p, mm, m, move);
			hole++;
			if(mdebug)fprintf(stderr,"%%ymin x %f %f %f cw %f ccw %f\n",
			  p->prev->ppoint.x,p->ppoint.x,p->next->ppoint.x,cw,ccw);
		}
		else if(fabs(p->ppoint.y-mm->sm->ymax)<SMALLDIF&&p->next != NULL&&p->prev != NULL){
			if(p->ppoint.x >= p->next->ppoint.x &&
				p->prev->ppoint.x >= p->ppoint.x)ccw++;
			else if(p->ppoint.x <= p->next->ppoint.x &&
				p->prev->ppoint.x <= p->ppoint.x)cw++;
			if(move>1)checkit(p, mm, m, move);
			hole++;
			if(mdebug)fprintf(stderr,"%%ymax x %f %f %f cw %f ccw %f\n",
			  p->prev->ppoint.x,p->ppoint.x,p->next->ppoint.x,cw,ccw);
		}
		if(fabs(p->ppoint.x-mm->sm->xmin)<SMALLDIF&&p->next != NULL&&p->prev != NULL){
			if(p->ppoint.y >= p->next->ppoint.y &&
				p->prev->ppoint.y >= p->ppoint.y)ccw++;
			else if(p->ppoint.y <= p->next->ppoint.y &&
				p->prev->ppoint.y <= p->ppoint.y)cw++;
			if(move>1)checkit(p, mm, m, move);
			hole++;
			if(mdebug)fprintf(stderr,"%%xmin x %f %f %f cw %f ccw %f\n",
			  p->prev->ppoint.y,p->ppoint.y,p->next->ppoint.y,cw,ccw);
		}
		else if(fabs(p->ppoint.x-mm->sm->xmax)<SMALLDIF&&p->next != NULL&&p->prev != NULL){
			if(p->ppoint.y <= p->next->ppoint.y &&
				p->prev->ppoint.y <= p->ppoint.y)ccw++;
			else if(p->ppoint.y >= p->next->ppoint.y&&
				p->prev->ppoint.y >= p->ppoint.y)cw++;
			if(move>1)checkit(p, mm, m, move);
			hole++;
			if(mdebug)fprintf(stderr,"%%xmax x %f %f %f cw %f ccw %f\n",
			  p->prev->ppoint.y,p->ppoint.y,p->next->ppoint.y,cw,ccw);
		}
	}
	j=0;
	for(mm=m; mm<mp; mm++){
		if(mm->used)continue;
		if(mm->hashole){
			mm->sm->objct += mm->hashole;
			move -= mm->hashole;
			mm->used = 1;
			if(mdebug)fprintf(stderr,"%%%d hashole\n",mm-m);
			start[j] = mm->sm->sptr;
			if(direction[mm-m] == CW)
				newf = rev(mm,m, path);
			estart[j] = mm->sm->moves;
			interior[j++] = 0;
			mpm = mm;
			for(mx=m, mct=0; mx<mp && mct < mm->hashole; mx++){
				if(mx->obj == mm){
					mct++;
					mm->sm->count += mx->sm->count;
					if(mdebug)fprintf(stderr,"%%found hole %d for %d ct %d\n",
						mx-m, mm-m, mm->sm->count);
					mx->used = 2;
					start[j] = mx->sm->sptr;
					if(direction[mx-m] == CCW)
						newf = rev(mx,m,path);
					estart[j] = mx->sm->moves;
					interior[j++] = 1;
					if(mpm->close->next != mx->sm->moves){
						/*relink so part of mm*/
						mmcln = mpm->close->next;
						mxopp = mx->sm->moves->prev;
						mxcln = mx->close->next;
	if(mdebug)fprintf(stderr,"%%ptrs %x %x %x\n",mmcln,mxopp,mxcln);
						if(path->first == mx->sm->moves){
							path->first = mpm->sm->moves;
						}
						mpm->close->next = mx->sm->moves;
						mx->sm->moves->prev = mpm->close;
						mx->close->next = mmcln;
						if(mmcln != NULL)
							mmcln->prev = mx->close;
						if(mxopp != NULL)
							mxopp->next = mxcln;
						if(mxcln != NULL)
							mxcln->prev = mxopp;
					}
					mpm = mx;
				}
			}
		}
		else if(mm->obj){
			if(mdebug)fprintf(stderr,"%%skipping hole %d\n",mm-m);
			continue;
		}
		else {
			mm->used = 1;
			if(mdebug)fprintf(stderr,"%%no hole %d\n",mm-m);
			start[j] = mm->sm->sptr;
			if(direction[mm-m] == CW)
				newf = rev(mm,m,path);
			estart[j] = mm->sm->moves;
			interior[j++] = 0;
		}
	}
	if(mdebug){
	  for(j=0,mm=m;mm<mp;mm++)
	    fprintf(stderr,"%%used %d ct %d\n",mm->used, mm->sm->count);
	  fprintf(stderr,"%%move %d\n",move);
	  for(j=0,mm=m;mm<mp; j++,mm++)
	    fprintf(stderr,"%%%d b %d obj %d ct %d x %f %f y %f %f elem %x\n",
		mm->used,moves[j].bpath,moves[j].objct,moves[j].count,moves[j].xmin,
			moves[j].xmax,moves[j].ymin,moves[j].ymax,moves[j].moves);
	  for(j=0;j<k;j++){
	    fprintf(stderr,"%%interior[%d] = %d %x %x %f\n",j,interior[j],start[j],estart[j],start[j]->next->slope);
	  }
	}
	fflush(stderr);
	if(havehole){
		i=0;
		mx = mp;
		for(mm=m; mm<mp;mm++){
			if(mm->used == 1){
				if(mm != mx)i+=mm->sm->objct;
				continue;
			}
			for(mx=mm+1; mx<mp;mx++){
				if(mx->used == 1){
					mm->sm->moves = mx->sm->moves;
					mm->sm->sptr = mx->sm->sptr;
					mm->sm->bpath = i;
					i += mx->sm->objct;
					mm->sm->objct = mx->sm->objct;
					mm->sm->count = mx->sm->count;
					mm->sm->xmin = mx->sm->xmin;
					mm->sm->xmax = mx->sm->xmax;
					mm->sm->ymin = mx->sm->ymin;
					mm->sm->ymax = mx->sm->ymax;
					mm->used = 1;
					mx->used = 2;
	if(mdebug)fprintf(stderr,"%%copied %d to %d marking %d as 2\n",mx-m,mm-m,mx-m);
					break;
				}
			}
			if(mx == mp)break;
		}
		i = 0;
		for(j=0, mm=m; j<move; j++, mm++){
			p = mm->sm->moves;
			mct = mm->sm->objct;
			while(1){
				if(p->type == PA_MOVETO){
					if(start[i]->member != i){
					   for(ps=start[i];p->type!=PA_CLOSEPATH;ps=ps->next,p=p->next)
							ps->member = i;
						ps->member = i;
					}
					if(++i >= mct)break;
				}
				p = p->next;
			}
		}
	}
	for(j=1,mm=m;j<move;j++, mm++){
		moves[0].objct += moves[j].objct;
		moves[0].count += moves[j].count;
		if(moves[j].xmin < moves[0].xmin)moves[0].xmin = moves[j].xmin;
		if(moves[j].xmax > moves[0].xmax)moves[0].xmax = moves[j].xmax;
		if(moves[j].ymin < moves[0].ymin)moves[0].ymin = moves[j].ymin;
		if(moves[j].ymax > moves[0].ymax)moves[0].ymax = moves[j].ymax;
	}
	move=1;
	if(mdebug){
		for(j=0,mm=m;mm<mp;mm++)
		  fprintf(stderr,"%%used %d ct %d\n",mm->used, mm->sm->count);
		fprintf(stderr,"%%move %d\n",move);
		for(j=0,mm=m;j<move; j++,mm++)
		  fprintf(stderr,"%%%d b %d obj %d ct %d x %f %f y %f %f elem %x\n",
			mm->used,moves[j].bpath,moves[j].objct,moves[j].count,moves[j].xmin,
			moves[j].xmax,moves[j].ymin,moves[j].ymax,moves[j].moves);
		for(j=0;j<k;j++)
		 fprintf(stderr,"%%interior[%d] = %d %x %x memb %d\n",j,interior[j],
			start[j],estart[j],start[j]->member);
	}
	return(move);
}
void
checkit(struct element *p, struct working *mp, struct working *m, int moves)
{
	struct working *mm;
	int i, j;
	for(mm=m, i=0; i< moves; i++, mm++){
		if(mm==mp)continue;
		j=inside(p->ppoint, mm->sm->moves, mm->sm->sptr, 0, 1, 0, 0);
		if(j){
			mp->obj = mm;
			mp->hole++;
			if(mdebug)fprintf(stderr,"%%inside %d\n",mm-m);
		}
	}
}

void
recomp(struct element *p, struct lstats *ps)
{
	struct element *pp;
	double deltax;
	ps++;
	for(pp=p->next; pp!= NULL; pp=pp->next, ps++){
		if(pp->type == PA_MOVETO)break;
		ps->type = 0;
		deltax = pp->ppoint.x - pp->prev->ppoint.x;
		if(fabs(deltax) >= SMALLDIF){
			ps->slope = (pp->ppoint.y - pp->prev->ppoint.y)/deltax;
			if(fabs(ps->slope) < SMALLDIF)ps->type = HORIZ;
		}
		else ps->type = VERT;
	}
}
int
getinside(struct element *pstart, struct lstats *pstats[], int mct,int start,
	struct element *qstart, struct lstats *qstats[], int qct, int startq,
	int interior[],
	double mnx, double mny, double mxx, double mxy)
{
	struct element *p;
	struct lstats *ps;
	int firsttime=0, ct=0;
	for(p=pstart; p != NULL; p=p->next, ps=ps->next){
		if(p->type == PA_MOVETO){
			ps = pstats[start++];
			if(firsttime++ == mct)break;
		}
		if(!inoutline)ps->inside = 0;
		else{ ps->inside = 1;
			ct++;
		}
		if(((p->ppoint.x >=mnx && p->ppoint.x <=mxx)||
			fabs(p->ppoint.x-mnx)<SMALLDIF||
			fabs(p->ppoint.x-mxx)<SMALLDIF)
			&& ((p->ppoint.y >=mny && p->ppoint.y <=mxy)||
			fabs(p->ppoint.y-mny)<SMALLDIF||
			fabs(p->ppoint.y-mxy)<SMALLDIF)
			){
			if(inside(p->ppoint, qstart, 0, qstats, qct, startq, interior)){
				if(!inoutline){
					ps->inside = 1;
					ct++;
				}
				else{ ps->inside = 0;
					ct--;
				}
			}
		}
		if(mdebug){ 
			if(ps->inside){
	if(p->type == PA_LINETO)
 fprintf(stderr,"%f sx %f sy moveto (o)show %%p %x t %d p %x n %x st %x\n",
   p->ppoint.x,p->ppoint.y,p,p->type,p->prev,p->next,ps);
	else fprintf(stderr,"%f sx %f sy moveto (x)show %%p %x t %d p %x n %x st %x\n",
   p->ppoint.x,p->ppoint.y,p,p->type,p->prev,p->next,ps);

				}
			else 
	if(p->type == PA_LINETO)
 fprintf(stderr,"%f sx %f sy moveto (.)show %%p %x t %d p %x n %x st %x\n",
   p->ppoint.x,p->ppoint.y,p,p->type,p->prev,p->next,ps);
	else fprintf(stderr,"%f sx %f sy moveto (,)show %%p %x t %d p %x n %x st %x\n",
   p->ppoint.x,p->ppoint.y,p,p->type,p->prev,p->next,ps);
		}
	}
	return(ct);
}
void
findlast(struct element *qfirst, struct lstats *qs)
{
	struct element *q,*p,*q1;
	int firsttime = 0,found;
	for(q=qfirst; q!= NULL; q=q->next){
		if(q->type==PA_MOVETO)continue;
		if(qs->inside==1){
			found=0;
			for(p=Graphics.path.first;p != NULL;p=p->next){
				if(fabs(p->ppoint.x-q->prev->ppoint.x)<SMALLDIF &&
			 	fabs(p->ppoint.y-q->prev->ppoint.y)<SMALLDIF){
					found=1;
					break;
				}
			}
			if(!found){
			q1=(q->prev)->prev;
			if(q->prev->type != PA_LINETO)q->prev->type=PA_LINETO;
			for(p=Graphics.path.first;p!=NULL;p=p->next){
				if(p->type==PA_MOVETO)continue;
				if(q1 != 0 &&q1->ppoint.x==p->ppoint.x &&
				 q1->ppoint.y == p->ppoint.y){
					if(!p->next)
						add_element(pointtype,q->prev->ppoint);
					else insert_element(q->prev,p->next);
					found=1;
					break;
				}
				if(q->ppoint.x==p->ppoint.x && q->ppoint.y==p->ppoint.y){
					insert_element(q->prev,p);
					found=1;
					break;
				}
			}
			if(!found)add_element(pointtype,q->prev->ppoint);
			}
		}
		qs=qs->next;
	}
}
int
equalpath(struct path path1, struct path path2)
{
	struct element *p1, *p2;
	for(p1=path1.first,p2=path2.first; p1 != NULL && p2 != NULL;
		p1=p1->next,p2=p2->next){
		if(p1->ppoint.x != p2->ppoint.x)return(0);
		if(p1->ppoint.y != p2->ppoint.y)return(0);
	}
	return(1);
}
void
insert_element(struct element *new, struct element *place)
{
	struct element *element;
	element=get_elem();
	element->next = place ;
	element->prev = place->prev ;
	place->prev->next = element ;
	place->prev = element ;
	element->type=new->type;
	element->ppoint=new->ppoint;
}
void
printpath(char *ss, struct path path, int max, int plot)
{
	struct element *p;
	int i=0;
	fprintf(stderr,"%%%s: first %x last %x\n",ss,path.first,path.last);
	if(path.first==NULL){fprintf(stderr,"%%null\n");return;}
	if(plot)fprintf(stderr,"[] 0 setdash 0 setlinewidth newpath\n");
	for(p=path.first; p != NULL && i < max; p=p->next, i++){
		if(plot){
		switch(p->type){
		case PA_MOVETO:
			fprintf(stderr,"%f sx %f sy moveto ",p->ppoint.x,p->ppoint.y);
			break;
		case PA_CLOSEPATH:
			fprintf(stderr,"closepath ");
			break;
		case PA_LINETO:
			fprintf(stderr,"%f sx %f sy lineto ",p->ppoint.x,p->ppoint.y);
		}
		}
		fprintf(stderr,"%%	%d %d (%f %f) %x next %x prev %x\n",i,p->type, p->ppoint.x,
			p->ppoint.y,p,p->next,p->prev);
	}
	if(plot)fprintf(stderr,"stroke\n");
}
void
addpart(struct element *p, int mct)
{
	struct element *p1;
	int ct=0;
	for(p1=p; p1 != NULL; p1=p1->next){
		if(p1->type == PA_MOVETO){
			if(ct >= mct)break;
			ct++;
		}
		append_elem(p1);
	}
}
void
append_elem(struct element *p)
{
	struct element *np;
	np = get_elem();
	np->type = p->type;
	np->ppoint = p->ppoint;
	Graphics.path.last->next = np;
	np->prev = Graphics.path.last;		/*was ->prev*/
	np->next = NULL;
	Graphics.path.last = np;
}
void
reorder(void){
	struct pts *arr[MAXINTERS], *pt=inters, *wpt, *spt;
	struct element *q;
	int i, j;

	for(wpt=winters; wpt<pts; wpt++ ){
		if(wpt->group > 1 && wpt->q != 0){
			copystr(wpt,pt++);
			continue;
		}
		q = wpt->q;
		if(q == 0)
			continue;
		for(spt=wpt, i=0; spt<pts; spt++)
			if(spt->q == q && spt->group == 1){
				arr[i++] = spt;
	if(mdebug)fprintf(stderr,"%%found inters %d sp %x qd %f pd %f %f %f %d\n",i,spt,
	spt->qdist,spt->dist,spt->x,spt->y,spt->group);
			}
		if(i > 1)
			qsort(arr, i, sizeof(*arr), pcmp);
		for(j=0;j<i;j++){
			if(i>1 && j+1 < i && arr[j]->qdist == arr[j+1]->qdist)continue;
			copystr(arr[j],pt++);
		}
	}
	pts = pt;
	if(mdebug){
		for(pt=inters; pt<pts;pt++){
			fprintf(stderr,"%%inters %f %f qdist %f\n",pt->x,pt->y,pt->qdist);
		}
	}
}
int
pcmp(struct pts **q1, struct pts **q2)
{
	double d;
	d = (*q1)->qdist - (*q2)->qdist;
	if(d < 0.)return(-1);
	if(d == 0.)return(0);
	else return(1);
}

void
copystr(struct pts *qp, struct pts *pp)
{
	pp->p = qp->p;
	pp->q = qp->q;
	pp->qs = qp->qs;
	pp->ps = qp->ps;
	pp->x = qp->x;
	pp->y = qp->y;
	pp->pcode = qp->pcode;
	pp->qcode = qp->qcode;
	pp->qdist = qp->qdist;
	pp->dist = qp->dist;
	pp->group = qp->group;
	pp->elt = qp->elt;
	qp->q = 0;
}
void
printdata(struct element *p, struct lstats *ps, int movect)
{
	int beg=0;
	for(;p!=NULL && beg <= movect; p=p->next, ps=ps->next){
		if(p->type == PA_MOVETO)beg++;
		fprintf(stderr,"p %x ps %x ty %d pt %f %f sl %f\n",p,ps,p->type,
			p->ppoint.x,p->ppoint.y,ps->slope);
	}
}
void
addqhole(struct smoves *qm)
{
	struct element *q;
	struct lstats *qs;
	struct pts *pt;
	int i, addit,tang;

	for(i=qm->bpath; i<qm->bpath+qm->objct; i++){
		addit = tang = 0;
		for(q=qelement[i], qs=qst[i]; q->type != PA_CLOSEPATH;
		   q=q->next, qs=qs->next){
			if(!qs->inside){
				if(!tang && pts == inters){
					tang=1;
					continue;
				}
				addit = 1;
				break;
			}
			if(pts > inters)for(pt=inters; pt<pts; pt++)
				if(q == pt->q){
					addit = 1;
					break;
				}
			if(addit)break;
		}
		if(!addit)addpart(qelement[i],1);
	}
}
