#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

void
texture(Bitmap *db, Rectangle r, Bitmap *t, Fcode c)
{
	Layer *dl, *cover;

	dl = (Layer*)db;
	if(dl->cache==0){
    Easy:
		_texture(db, r, t, c);
		return;
	}
	if(rectclip(&r, dl->clipr) == 0)
		return;
	if(dl->vis==Visible)
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
	 * Do the operation in the cache and restore as necessary.
	 * If dl is Visible, r is contained within cover->clipr and
	 * the cache must be restored from the cover; otherwise the
	 * cache is up to date but the restoration of the screen may
	 * require clipping.
	 */
	cover = dl->cover->layer;
	if(dl->vis == Visible)
		bitblt(dl->cache, r.min, cover, r, S);
	_texture(dl->cache, r, t, c);
	if(dl->vis == Visible)
		bitblt(cover, r.min, dl->cache, r, S);
	else if(dl->vis==Obscured){
		if(rectclip(&r, cover->clipr))
			layerop(lupdate, r, dl, (void *)0);
	}
}

void
_texture(Bitmap *d, Rectangle r, Bitmap *s, Fcode f)
{
	uchar *buf;

	buf = bneed(23);
	buf[0] = 't';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, r.min.x);
	BPLONG(buf+7, r.min.y);
	BPLONG(buf+11, r.max.x);
	BPLONG(buf+15, r.max.y);
	BPSHORT(buf+19, s->id);
	BPSHORT(buf+21, f);
}
