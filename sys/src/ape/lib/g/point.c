#include <libg.h>

typedef unsigned char uchar;

void
point(Bitmap *d, Point p, int v, Fcode f)
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
