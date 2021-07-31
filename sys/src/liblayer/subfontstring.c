#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

Point
subfontstring(Bitmap *db, Point p, Subfont *f, char *s, Fcode c)
{
	Layer *dl, *cover;
	Rectangle r, rc;
	int fits;
	Point sz;

	dl = (Layer*)db;
	sz = subfontstrsize(f, s);
	if(dl->cache == 0)
    Easy:
		return _subfontstring(db, p, f, s, c, sz.x);
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
	_subfontstring(dl->cache, p, f, s, c, sz.x);
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
_subfontstring(Bitmap *d, Point p, Subfont *ft, char *s, Fcode f, int w)
{
	int c, i;
	uchar *buf;
	char *q, *eq;
	Rune r;
	int n;

	while(*s){
		q = memchr(s, 0, 1024);
		if(q == 0){
			w = -1;
			/*
			 * Must find a rune boundary.
			 * First try: look for a self-representing
			 * character near the end of the first 1024
			 */
			eq = s + 1023-3;	/* longest rune is 3 bytes */
			q = s + 900;
			while(q < eq){
				c = *(uchar*)q++;
				if(c <= ' ')
					goto found;
			}
			/*
			 * Second try: interpret runes from start
			 */
			for(q=s; q<eq; ){
				i = chartorune(&r, q);
				if(i == 0){
					q++;
					continue;
				}
				q += i;
			}
		}
	found:
		n = q - s;
		buf = bneed(15+n+1);
		buf[0] = 's';
		BPSHORT(buf+1, d->id);
		BPLONG(buf+3, p.x);
		BPLONG(buf+7, p.y);
		BPSHORT(buf+11, ft->id);
		BPSHORT(buf+13, f);
		memmove(buf+15, s, n);
		buf[15+n] = 0;
		if(w > 0)
			p.x += w;
		else{
			p.x += subfontstrwidth(ft, (char*)buf+15);
			w = -1;
		}
		s += n;
	}
	return p;
}
