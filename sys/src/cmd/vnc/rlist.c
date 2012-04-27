#include "vnc.h"
#include "vncs.h"

static int tot;
static void rprint(Rlist*);

static void
growrlist(Rlist *rlist, int n)
{
	int old;

	if(rlist->nrect+n <= rlist->maxrect)
		return;

	old = rlist->maxrect;
	while(rlist->nrect+n > rlist->maxrect){
		if(rlist->maxrect == 0)
			rlist->maxrect = 16;
		else
			rlist->maxrect *= 2;
	}

	tot += rlist->maxrect - old;
	if(tot > 10000)
		sysfatal("too many rectangles");

	rlist->rect = realloc(rlist->rect, rlist->maxrect*sizeof(rlist->rect[0]));
	if(rlist->rect == nil)
		sysfatal("realloc failed in growrlist");
}

static void
rappend(Rlist *rl, Rectangle r)
{
	growrlist(rl, 1);
	rl->rect[rl->nrect++] = r;
}

/* remove rectangle i from the list */
static int
rtrim(Rlist *r, int i)
{
	if(i < 0 || i >= r->nrect)
		return 0;
	if(i == r->nrect-1){
		r->nrect--;
		return 1;
	}
	r->rect[i] = r->rect[--r->nrect];
	return 1;
}

static int
rectadjacent(Rectangle r, Rectangle s)
{
	return r.min.x<=s.max.x && s.min.x<=r.max.x &&
	       r.min.y<=s.max.y && s.min.y<=r.max.y;
}

/*
 * If s shares three edges with r, compute the
 * rectangle r - s and return 1.
 * Else return 0.
 */
static int
rectubr(Rectangle *r, Rectangle s)
{
	if(r->min.y==s.min.y && r->max.y==s.max.y){
		if(r->min.x == s.min.x){
			r->min.x = s.max.x;
			assert(r->max.x > r->min.x);
			return 1;
		}
		if(r->max.x == s.max.x){
			r->max.x = s.min.x;
			assert(r->max.x > r->min.x);
			return 1;
		}
	}
	if(r->min.x==s.min.x && r->max.x==s.max.x){
		if(r->min.y == s.min.y){
			r->min.y = s.max.y;
			assert(r->max.y > r->min.y);
			return 1;
		}
		if(r->max.y == s.max.y){
			r->max.y = s.min.y;
			assert(r->max.y > r->min.y);
			return 1;
		}
	}
	return 0;
}

/*
 * If s is a corner of r, remove s from r, yielding
 * two rectangles r and rr.  R holds the part with
 * smaller coordinates.
 */
static int
rectcornersubr(Rectangle *r, Rectangle s, Rectangle *rr)
{
#	define UPRIGHT(r) Pt((r).max.x, (r).min.y)
#	define LOWLEFT(r) Pt((r).min.x, (r).max.y)

	*rr = *r;

	if(s.min.x == r->min.x){  
		if(s.min.y == r->min.y){  // upper left
			*rr = Rpt(UPRIGHT(s), r->max);
			*r = Rpt(LOWLEFT(s), LOWLEFT(*rr));
			return 1;
		}
		if(s.max.y == r->max.y){ // lower left
			*rr = Rpt(Pt(s.max.x, r->min.y), r->max);
			*r = Rpt(r->min, UPRIGHT(s));
			return 1;
		}
	}
	if(s.max.x == r->max.x){
		if(s.max.y == r->max.y){ // lower right
			*rr = Rpt(Pt(s.min.x, r->min.y), UPRIGHT(s));
			*r = Rpt(r->min, LOWLEFT(s));
			return 1;
		}
		if(s.min.y == r->min.y){ // upper right
			*rr = Rpt(LOWLEFT(s), r->max);
			*r = Rpt(r->min, LOWLEFT(*rr));
			return 1;
		}
	}
	return 0;
}

/*
 * If s is a band cutting r into two pieces, set r to one piece
 * and rr to the other.
 */
static int
recttridesubr(Rectangle *nr, Rectangle s, Rectangle *rr)
{
	*rr = *nr;
	if((nr->min.x == s.min.x && nr->max.x == s.max.x) &&
	    (nr->min.y < s.min.y && s.max.y < nr->max.y)){
		nr->max.y = s.min.y;
		rr->min.y = s.max.y;
		return 1;
	}

	if((nr->min.y == s.min.y && nr->max.y == s.max.y) &&
	    (nr->min.x < s.min.x && s.max.x < nr->max.x)){
		nr->max.x = s.min.x;
		rr->min.x = s.max.x;
		return 1;
	}
	return 0;
}

void
addtorlist(Rlist *rlist, Rectangle r)
{
	int i, j;
	Rectangle ir, cr, rr;
	Rlist tmp;

	if(r.min.x >= r.max.x || r.min.y >= r.max.y)
		return;

	memset(&tmp, 0, sizeof tmp);
	rappend(&tmp, r);
	
	if(verbose > 5)
		fprint(2, "region union add %R:\n", r);

	combinerect(&rlist->bbox, r); // must do this first
	for(j = 0; j < tmp.nrect; j++){
		r = tmp.rect[j];

		for(i=0; i < rlist->nrect; i++){
			ir = rlist->rect[i];

			if(verbose > 5)
				fprint(2, "checking %R against %R\n", r, ir);
			if(!rectadjacent(ir, r))
				continue;

			/* r is covered by ir? */
			if(rectinrect(r, ir))
				break;

			/* r covers ir? */
 			if(rectinrect(ir, r)){
				rtrim(rlist, i);
				i--;
				continue;
			}

			/* aligned and overlapping? */
			if((ir.min.y == r.min.y && ir.max.y == r.max.y) ||
		    	(ir.min.x == r.min.x && ir.max.x == r.max.x)){
				combinerect(&r, ir);
				rtrim(rlist, i);
				i--;
				continue;
			}

			/* not aligned */ 
			if(verbose > 5)
				fprint(2, "break up rect %R and %R\n", ir, r);
			/* 2->2 breakup */
			cr = ir;
			if (!rectclip(&cr, r))	/* share only one point */
				continue;

			if(rectubr(&r, cr))
				continue;

			if(rectubr(&rlist->rect[i], cr))
				continue;

			/* 2 -> 3 breakup */
			/* stride across */
			if(recttridesubr(&r, cr, &rr)){
				rappend(&tmp, rr);
				continue;
			}

			/* corner overlap */
			if(rectcornersubr(&r, cr, &rr)){
				rappend(&tmp, rr);
				continue;
			}
			abort();
		}
		if(i == rlist->nrect)
			rappend(rlist, r);
	}
	freerlist(&tmp);
	if(verbose > 5)
		rprint(rlist);
}

void
freerlist(Rlist *r)
{
	free(r->rect);
	tot -= r->maxrect;
	r->nrect = 0;
	r->maxrect = 0;
	r->rect = nil;
}

static void
rprint(Rlist *r)
{
	int i;

	fprint(2, "rlist %p:", r);
	for(i=0; i<r->nrect; i++)
		fprint(2, " %R", r->rect[i]);
	fprint(2, "\n");
}


#ifdef REGION_DEBUG

int verbose = 10;

void main(int argc, char * argv[])
{
	Rectangle r1 = Rect(0, 0, 300, 200);
	Rectangle r2 = Rect(100, 100, 400, 300);
	Rectangle r3 = Rect(200, 100, 500, 300);
	Region reg;

	initdraw(0, 0, "vncviewer");
	region_init(&reg);
	region_union(&reg, r1, r1);
	region_union(&reg, r2, r2);
	region_union(&reg, r3, r3);
}

#endif
