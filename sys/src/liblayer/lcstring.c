#include <u.h>
#include <libc.h>
#include <libg.h>
#include <layer.h>

	/*
	 * string
	 *
	 *	'u'		1
	 *	id		2
	 *	pt		8
	 *	font id		2
	 *	code		2
	 *	n		2
	 * 	cache indexes	2*n (not null terminated)
	 */

void
_lcstring(int id, uchar *s, int nindex)
{
	uchar *p;
	int n;

	n = 17+2*nindex;
	p = bneed(n);
	memcpy(p, s, n);
	BPSHORT(p+1, id);
}

/*
 * db is known to be a layer; deal with bitmap clipping in the
 * /dev/bitblt message to draw the string
 */
void
lcstring(Bitmap *db, int height, int nwidth, uchar *width, uchar *s, int nindex)
{
	Layer *dl, *cover;
	Rectangle r, rc;
	int wid;
	int i, j, fits;
	Point pt;

	dl = (Layer*)db;
	if(dl->cache == 0){
    Easy:
		_lcstring(db->id, s, nindex);
		return;
	}
	wid = 0;
	for(i=0; i<nindex; i++){
		j = BGSHORT(s+17+2*i);
		if(j<0 || j>=nwidth)
			continue;
		wid += width[j];
	}
	pt.x = BGLONG(s+3);
	pt.y = BGLONG(s+7);
	r.min = pt;
	r.max.x = r.min.x+wid;
	r.max.y = r.min.y+height;
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
		return;
	/* r is now bbox of the portion of dl that will be modified */
	cover = dl->cover->layer;
	if(dl->vis == Visible)
		bitblt(dl->cache, r.min, cover, r, S);
	if(!fits){
		rc = dl->cache->clipr;
		clipr(dl->cache, r);
	}
	_lcstring(dl->cache->id, s, nindex);
	if(!fits)
		clipr(dl->cache, rc);
	if(dl->vis == Visible)
		bitblt(cover, r.min, dl->cache, r, S);
	else if(dl->vis==Obscured)
		layerop(lupdate, r, dl, (void *)0);
}
