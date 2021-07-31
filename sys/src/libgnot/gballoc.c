#include <u.h>
#include <libc.h>
#include <libg.h>
#include <gnot.h>

#define OKRECT(r)	((r).min.x<(r).max.x && (r).min.y<(r).max.y)

GBitmap *
gballoc(Rectangle r, int ldepth)
{
	long lsize;
	int wsize;	/* word size in pixels */
	GBitmap *bp;

	if(ldepth<0 || ldepth>3 || !OKRECT(r))
		return 0;
	wsize = 1<<(5-ldepth);	/* ldepth==0 is 32 pixels per word */
	if(r.max.x >= 0)
		lsize = (r.max.x+wsize-1)/wsize;
	else
		lsize = r.max.x/wsize;
	if(r.min.x >= 0)
		lsize -= r.min.x/wsize;
	else
		lsize -= (r.min.x-wsize+1)/wsize;
	if((bp=malloc(sizeof(GBitmap))) == 0)
		return 0;
	bp->zero = lsize*r.min.y;
	if(r.min.x >= 0)
		bp->zero += r.min.x/wsize;
	else
		bp->zero += (r.min.x-wsize+1)/wsize;
	bp->zero = -bp->zero;
	bp->width = lsize;
	bp->ldepth = ldepth;
	bp->r = r;
	bp->clipr = r;
	bp->cache = 0;
	lsize *= Dy(r);
	lsize *= sizeof(ulong);
	bp->base = malloc(lsize);
	if(bp->base == 0){
		free(bp);
		return 0;
	}
	memset(bp->base, 0, lsize);
	return bp;
}

void
gbfree(GBitmap *bp)
{
	if(bp){
		if(bp->base)
			free(bp->base);
		free(bp);
	}
}
