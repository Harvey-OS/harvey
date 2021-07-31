#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
point(Bitmap *db, Point pt, int v, Fcode c)
{
	Layer *dl, *cover;
	Rectangle r;

	dl = (Layer*)db;
	if(dl->cache == 0){
    Easy:
		_point(db, pt, v, c);
		return;
	}
	if(!ptinrect(pt, dl->clipr))
		return;
	if(dl->vis == Visible)
		/*
		 * Look for a Bitmap we can write on instead of a Layer.
		 * pt is known to be within dl->clipr.  As long as we have
		 * only Visible covering layers, we know that pt will
		 * therefore be within their cliprs and no more clipping
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
	 * Do the operation in the cache and restore as necessary.
	 * If dl is Visible, pt is inside cover->clipr and
	 * the cache must be restored from the cover; otherwise the
	 * cache is up to date but the restoration of the screen may
	 * require clipping.
	 */
	r = Rect(pt.x, pt.y, pt.x+1, pt.y+1);
	cover = dl->cover->layer;
	if(dl->vis == Visible)
		bitblt(dl->cache, r.min, cover, r, S);
	_point(dl->cache, pt, v, c);
	if(dl->vis == Visible)
		bitblt(cover, r.min, dl->cache, r, S);
	else if(dl->vis == Obscured){
		if(ptinrect(pt, cover->clipr))
			layerop(lupdate, r, dl, (void *)0);
	}
}

void
_point(Bitmap *d, Point p, int v, Fcode f)
{
	uchar *buf;

	buf = bneed(14);
	buf[0] = 'p';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, p.x);
	BPLONG(buf+7, p.y);
	buf[11] = v;
	BPSHORT(buf+12, f);
}
