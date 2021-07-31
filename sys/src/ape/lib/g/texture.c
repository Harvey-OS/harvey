#include <libg.h>

typedef unsigned char uchar; 

void
texture(Bitmap *d, Rectangle r, Bitmap *s, Fcode f)
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
