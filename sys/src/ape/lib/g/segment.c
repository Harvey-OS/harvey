#include <libg.h>

typedef unsigned char uchar;

void
segment(Bitmap *d, Point p1, Point p2, int v, Fcode f)
{
	uchar *buf;

	buf = bneed(22);
	buf[0] = 'l';
	BPSHORT(buf+1, d->id);
	BPLONG(buf+3, p1.x);
	BPLONG(buf+7, p1.y);
	BPLONG(buf+11, p2.x);
	BPLONG(buf+15, p2.y);
	buf[19] = v;
	BPSHORT(buf+20, f);
}
