#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
segment(Bitmap *db, Point pt1, Point pt2, int v, Fcode c)
{
	Layer *dl, *cover;
	Rectangle r, rc;
	Point t1, t2;
	int fits;

	dl = (Layer*)db;
	if(dl->cache == 0){
    Easy:
		_segment(db, pt1, pt2, v, c);
		return;
	}
	/* this is clumsy but safe */
	r = rcanon(Rpt(pt1, pt2));
	if(pt2.x > pt1.x)
		r.max.x++;
	if(pt2.y > pt1.y)
		r.max.y++;
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
			if(cover->vis != Visible)
				break;
			if(cover->cache == 0){
				db = cover;
				goto Easy;
			}
		}
	/*
	 * Either dl or one of the covering layers is not Visible.
	 * Do the operation in the cache and restore as necessary.
	 * If dl is Visible, r is contained within cover->clipr and
	 * the cache must be restored from the cover; otherwise the
	 * cache is up to date but the restoration of the screen may
	 * require clipping.
	 *
	 * First we must do real line clipping; if we were lucky we
	 * avoided it altogether and didn't even get here.
	 */
	t1 = pt1;
	t2 = pt2;
	if(!clipline(dl->clipr, &t1, &t2))
		return;
	r = rcanon(Rpt(t1, t2));
	r.max.x++;
	r.max.y++;
	if(!fits && !rectclip(&r, dl->clipr))
		return;
	/* r is now bbox of the portion of dl that will be modified */
	cover = dl->cover->layer;
	if(dl->vis == Visible)
		bitblt(dl->cache, r.min, cover, r, S);
	if(!fits){
		rc = dl->cache->clipr;
		clipr(dl->cache, r);
	}
	_segment(dl->cache, pt1, pt2, v, c);
	if(!fits)
		clipr(dl->cache, rc);
	if(dl->vis == Visible)
		bitblt(cover, r.min, dl->cache, r, S);
	else if(dl->vis==Obscured)
		layerop(lupdate, r, dl, (void *)0);
}

void
_segment(Bitmap *d, Point p1, Point p2, int v, Fcode f)
{
	uchar *buf;

	buf = bneed(22);
	buf[0] = 'l';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, p1.x);
	BPLONG(buf+7, p1.y);
	BPLONG(buf+11, p2.x);
	BPLONG(buf+15, p2.y);
	buf[19] = v;
	BPSHORT(buf+20, f);
}
