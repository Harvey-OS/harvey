#include <u.h>
#include <libc.h>
#include <libg.h>

void
bitblt(Bitmap *d, Point p, Bitmap *s, Rectangle r, Fcode f)
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
