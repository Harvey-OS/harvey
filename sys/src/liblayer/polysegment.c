#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
polysegment(Bitmap *db, int n, Point *pp, int v, Fcode c)
{
	Layer *dl, *cover;
	Rectangle r;
	Point *q;
	int i, fits;

	dl = (Layer*)db;
	if(n < 2)
		return;
	if(dl->cache == 0){
    Easy:
		_polysegment(db, n, pp, v, c);
		return;
	}
	/* this is clumsy but safe */
	r = Rpt(pp[0], pp[0]);
	for(q=pp+1,i=1; i<n; i++,q++){
		if(q->x < r.min.x)
			r.min.x = q->x;
		if(q->x > r.max.x)
			r.max.x = q->x;
		if(q->y < r.min.y)
			r.min.y = q->y;
		if(q->y > r.max.y)
			r.max.y = q->y;
	}
	r = inset(r, -1);	/* breathing room for the clip */
	fits = rectinrect(r, dl->clipr);
	if(fits && dl->vis==Visible)
		/*
		 * Look for a Bitmap we can write on instead of a Layer.
		 * r is known to fit within dl->clipr.  As long as we have
		 * only Visible covering layers, we know that r will
		 * therefore fit within their cliprs and no more clipping
		 * is required.
		 */
		for(cover=dl->cover->layer; ; cover=cover->cover->layer){
			if(cover->cache == 0){
				db = cover;
				goto Easy;
			}
			if(cover->vis != Visible)
				break;
		}
	/*
	 * Either dl or one of the covering layers is not Visible.
	 * Clipping polysegment is too hard; just call segment;
	 * if we were lucky we didn't even get here.
	 */
	for(i=0; i<n-1; i++)
		segment(dl, pp[i], pp[i+1], v, c);
}

void
_polysegment(Bitmap *d, int n, Point *pp, int v, Fcode f)
{
	int i, m;
	uchar *buf;
	Point *p1, *p2;

	n--;	/* n is now number of lines to draw */
	while(n > 0){
		p1 = pp;
		p2 = pp+1;
		for(m=0; m<n; m++,p1++,p2++){
			if(m == 2000)
				break;
			if(p2->x-p1->x < -128 || p2->x-p1->x > 127)
				break;
			if(p2->y-p1->y < -128 || p2->y-p1->y > 127)
				break;
		}
		/* m is number of short lines to draw */
		if(m == 0){
			segment(d, pp[0], pp[1], v, f);
			pp++;
			n--;
			continue;
		}
		buf = bneed(16+2*m);
		buf[0] = 'e';
		BPSHORT(buf+1, d->id);
		BPLONG(buf+3, pp[0].x);
		BPLONG(buf+7, pp[0].y);
		buf[11] = v;
		BPSHORT(buf+12, f);
		BPSHORT(buf+14, m);
		buf += 16;
		p2 = pp+1;
		for(i=0; i<m; i++,pp++,p2++){
			*buf++ = p2->x - pp->x;
			*buf++ = p2->y - pp->y;
		}
		n -= m;
	}
}
