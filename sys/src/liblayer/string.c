#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

Point
string(Bitmap *db, Point p, Font *f, char *s, Fcode c)
{
	Layer *dl, *cover;
	Rectangle r, rc;
	Point sz;
	int fits;

	dl = (Layer*)db;
	if(dl->cache == 0)
    Easy:
		return _string(db, p, f, s, c);
	sz = strsize(f, s);
	r.min = p;
	r.max.x = p.x+sz.x;
	r.max.y = p.y+sz.y;
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
	 */
	if(!fits && !rectclip(&r, dl->clipr))
		goto Return;
	/* r is now bbox of the portion of dl that will be modified */
	cover = dl->cover->layer;
	if(dl->vis == Visible)
		bitblt(dl->cache, r.min, cover, r, S);
	if(!fits){
		rc = dl->cache->clipr;
		clipr(dl->cache, r);
	}
	_string(dl->cache, p, f, s, c);
	if(!fits)
		clipr(dl->cache, rc);
	if(dl->vis == Visible)
		bitblt(cover, r.min, dl->cache, r, S);
	else if(dl->vis==Obscured)
		layerop(lupdate, r, dl, (void *)0);
    Return:
	return Pt(p.x+sz.x, p.y);
}

Point
_string(Bitmap *d, Point p, Font *f, char *s, Fcode fc)
{
	int n, wid;
	uchar *b;
	ushort *c, *ec;
	enum { Max = 128 };
	static ushort cbuf[2*Max];

	while(*s){
		n = cachechars(f, &s, cbuf, Max, &wid);
		b = bneed(17+2*n);
		b[0] = 's';
		BPSHORT(b+1, d->id);
		BPLONG(b+3, p.x);
		BPLONG(b+7, p.y);
		BPSHORT(b+11, f->id);
		BPSHORT(b+13, fc);
		BPSHORT(b+15, n);
		b += 17;
		ec = &cbuf[n];
		for(c=cbuf; c<ec; c++){
			BPSHORT(b, *c);
			b += 2;
		}
		p.x += wid;
	}
	agefont(f);
	return p;
}
