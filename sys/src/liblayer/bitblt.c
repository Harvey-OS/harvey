#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
bitblt(Bitmap *db, Point p, Bitmap *sb, Rectangle r, Fcode c)
{
	Layer *dl=(Layer *)db, *sl=(Layer *)sb;
	int dok;
	Rectangle dr;

	if(!bitbltclip(&db))
		return;
	/*
	 * Look for a Bitmap we use for source instead of a Layer.
	 * r now fits within sl->clipr.  As long as we have
	 * only Visible covering layers, we know that r will
	 * therefore fit within their cliprs and no more clipping
	 * is required.
	 */
	while(sl->cache){
		if(sl->vis != Visible){
			sb = sl->cache;
			break;
		}
		sl = sl->cover->layer;
		sb = sl;
	}
	/*
	 * Do the same for the destination
	 */
	dok = 1;
	while(dl->cache){
		if(dl->vis != Visible){
			db = dl->cache;
			dok = 0;
			break;
		}
		dl = dl->cover->layer;
		db = dl;
	}

	/*
	 * sb is now directly usable.  Two cases remain: db Visible or not
	 */
	if(dok){
		/*
		 * Bitmap to bitmap, easy.
		 */
		_bitblt(db, p, sb, r, c);
		return;
	}
	/*
	 * Destination not Visible; use destination cache
	 */
	dr.min = p;
	dr.max.x = p.x + Dx(r);
	dr.max.y = p.y + Dy(r);
	_bitblt(dl->cache, p, sb, r, c);
	if(dl->vis == Obscured)
		layerop(lupdate, dr, dl, (void *)0);
	return;
}

void
_bitblt(Bitmap *d, Point p, Bitmap *s, Rectangle r, Fcode f)
{
	uchar *buf;

	buf = bneed(31);
	buf[0] = 'b';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, p.x);
	BPLONG(buf+7, p.y);
	BPSHORT(buf+11, s->id);
	BPLONG(buf+13, r.min.x);
	BPLONG(buf+17, r.min.y);
	BPLONG(buf+21, r.max.x);
	BPLONG(buf+25, r.max.y);
	BPSHORT(buf+29, f);
}
