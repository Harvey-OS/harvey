#include <u.h>
#include <libc.h>
#include <libg.h>

int
clipr(Bitmap *d, Rectangle r)
{
	uchar *buf;

	if(rectclip(&r, d->r) == 0)
		return 0;
	buf = bneed(19);
	buf[0] = 'q';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, r.min.x);
	BPLONG(buf+7, r.min.y);
	BPLONG(buf+11, r.max.x);
	BPLONG(buf+15, r.max.y);
	d->clipr = r;
	return 1;
}
