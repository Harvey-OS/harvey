#include <u.h>
#include <libc.h>
#include <draw.h>

#include "region.h"

extern int verbose;

#define OKRECT(r) ((r).min.x < (r).max.x && (r).min.y < (r).max.y)

extern void region_print(Region *);

static tot_rects = 0;

static void region_alloc(RegData * pReg, int howmany)
{
	int old = pReg->size;

	if(  pReg->nrects + howmany <= pReg->size )
		return;

	do {
		if( pReg->size == 0 ) 
			pReg->size = 16;
		else
			pReg->size <<= 1;
	} while( pReg->nrects + howmany >= pReg->size );

	tot_rects += pReg->size - old;
	if( tot_rects > 10000 ) {
		fprint(2, "too many rectangles\n");
		while(1) sleep(10000);
		abort();
	}
	pReg->rects = realloc( pReg->rects, pReg->size*sizeof(Rectangle) );

	if( pReg->rects == nil )
		sysfatal("region_apprend: realloc failed %r\n");
}

static void region_append(Region * pReg, Rectangle r)
{
	region_alloc(&pReg->RegData, 1);
	pReg->rects[ pReg->nrects++ ] = r;
}

void region_init(Region * pReg)
{
	pReg->extents = Rect(0,0,0,0);
	pReg->size = pReg->nrects = 0;
	pReg->rects = nil;
}

/* trim the ith item */
int region_trim(Region * pReg, int i)
{
	int last = pReg->nrects-1;

	if( last < 0 || i < 0 || i > last )
		return 0;
	// assert( last >=0 && && i >= 0 && i <= last );

	if( i != last )
		pReg->rects[i] = pReg->rects[last];
	pReg->nrects--;
	return 1;
}

/* adjacency check, differ from rectXrect by one pixel */
static int
rectadjacent(Rectangle r, Rectangle s)
{
	return r.min.x<=s.max.x && s.min.x<=r.max.x &&
	       r.min.y<=s.max.y && s.min.y<=r.max.y;
}

/* s is inside of nr share one edge.
 * if Dz(s) == Dz(*nr), then
 * compute *nr = *nr - s
*/
static int
rectsubr(Rectangle *nr, Rectangle s)
{
	if( nr->min.y == s.min.y && nr->max.y == s.max.y ) {
		if( nr->min.x == s.min.x ) {
			nr->min.x = s.max.x;
			assert(nr->max.x > nr->min.x);
			return 1;
		}
		if( nr->max.x == s.max.x ) {
			nr->max.x = s.min.x;
			assert(nr->max.x > nr->min.x);
			return 1;
		}
	}
	if( nr->min.x == s.min.x && nr->max.x == s.max.x ) {
		if( nr->min.y == s.min.y ) {
			nr->min.y = s.max.y;
			assert(nr->max.y > nr->min.y);
			return 1;
		}
		if( nr->max.y == s.max.y ) {
			nr->max.y = s.min.y;
			assert(nr->max.y > nr->min.y);
			return 1;
		}
	}
	return 0;
}

#define UPRIGHT(r) Pt((r).max.x, (r).min.y)
#define LOWLEFT(r) Pt((r).min.x, (r).max.y)

/* s is a corner of nr, Dz(s) < Dz(*nr) for z=x,y
 * cut it so that *nr = (*nr - s - *rr)
 * cut along Y, *nr holds the left part
 */
static int
rectcornersubr(Rectangle *nr, Rectangle s, Rectangle *rr)
{
	*rr = *nr;

	if( s.min.x == nr->min.x ) {  
		if( s.min.y == nr->min.y ) {  // upper left
			*rr = Rpt(UPRIGHT(s), nr->max);
			*nr = Rpt(LOWLEFT(s), LOWLEFT(*rr));
			return 1;
		}
		if( s.max.y == nr->max.y ) { // lower left
			*rr = Rpt(Pt(s.max.x, nr->min.y), nr->max);
			*nr = Rpt(nr->min, UPRIGHT(s));
			return 1;
		}
	}
	if( s.max.x == nr->max.x ) {
		if( s.max.y == nr->max.y ) { // lower right
			*rr = Rpt(Pt(s.min.x, nr->min.y), UPRIGHT(s));
			*nr = Rpt(nr->min, LOWLEFT(s));
			return 1;
		}
		if( s.min.y == nr->min.y ) { // upper right
			*rr = Rpt(LOWLEFT(s), nr->max);
			*nr = Rpt(nr->min, LOWLEFT(*rr));
			return 1;
		}
	}
	return 0;
}

/* check if s is a band cutting nr in two pieces, Dz(s) == Dz(*nr) for *one* of x and y
 * if so, cut it so that *nr = (*nr - s - *rr)
 */

static int
rectstridesubr(Rectangle *nr, Rectangle s, Rectangle *rr)
{
	*rr = *nr;
	if( (nr->min.x == s.min.x && nr->max.x == s.max.x) &&
	    (nr->min.y < s.min.y && s.max.y < nr->max.y) ) {
		nr->max.y = s.min.y;
		rr->min.y = s.max.y;
		return 1;
	}

	if( (nr->min.y == s.min.y && nr->max.y == s.max.y) &&
	    (nr->min.x < s.min.x && s.max.x < nr->max.x) ) {
		nr->max.x = s.min.x;
		rr->min.x = s.max.x;
		return 1;
	}
	return 0;
}

/* allow partial overlaping */
void region_union(Region * pReg, Rectangle r, Rectangle clipr)
{
	int i, j;
	Rectangle ir, cr, rr;
	Region rreg;

	if( ! OKRECT(r) )
		return;
	if( !rectclip(&r, clipr) )
		return;

	if( verbose > 5 )
		fprint(2, "region union add %R:\n", r);

	region_init(&rreg);
	region_append(&rreg, r);

	combinerect(&pReg->extents, r); // must do this first
	for(j = 0; j < rreg.nrects; j++) {
		r = rreg.rects[j];

		for(i=0; i < pReg->nrects; i++) {
			ir = pReg->rects[i];

			if(verbose > 5)
				fprint(2, "checking %R against %R\n", r, ir);
			if( ! rectadjacent(ir, r) )
				continue;

			if( rectinrect(r, ir) ) // r is covered
				break;

 			if( rectinrect(ir, r) ) { // r covers
				region_trim(pReg, i);
				i--;  // scan the new item in i or breakout
				continue;
			}

			// X/Y aligned and overlaping
			if( (ir.min.y == r.min.y && ir.max.y == r.max.y ) ||
		    	(ir.min.x == r.min.x && ir.max.x == r.max.x ) ) {
				combinerect(&r, ir);
				region_trim(pReg, i);
				i--;
				continue;
			}

			// not aligned 
if( verbose > 5 ) fprint(2, "break up rect %R and %R\n", ir, r);
			// 2 -> 2 breakup
			cr = ir;
			if ( !rectclip(&cr, r) )  // share one point only
				continue;

			if( rectsubr(&r, cr) )
				continue;

			if( rectsubr(&pReg->rects[i], cr) )
				continue;

			// 2 -> 3 breakup
			// stride across
			if( rectstridesubr(&r, cr, &rr) ) {
				region_append(&rreg, rr);
				continue;
			}

			// corner overlap
			if( rectcornersubr(&r, cr, &rr) ) {
				region_append(&rreg, rr);
				continue;
			}
			sysfatal("should not be here\n");
		}
		if( i == pReg->nrects )
			region_append(pReg, r);
	}
	region_reset(&rreg);
	if(verbose > 5)
		region_print(pReg);
}

void region_reset(Region * pReg)
{
	tot_rects -= pReg->size;

	if( pReg->size )
		free(pReg->rects);
	region_init(pReg);
}

/* make a copy, override dst */
void region_copy(Region * dst, Region *src)
{
	region_alloc(&dst->RegData, src->nrects - dst->nrects);

	memcpy(dst->rects, src->rects, src->nrects * sizeof(Rectangle));
	dst->extents = src->extents;
	dst->nrects = src->nrects;
}


void region_print(Region * pReg)
{
	int i;

	fprint(2, "region %p: ", pReg);
	for(i = 0; i < pReg->nrects; i++) {
		fprint(2, "%R ", pReg->rects[i]);
	}
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
