#include <libg.h>

typedef unsigned char uchar;

void
cursorset(Point p)
{
	uchar *buf;

	buf = bneed(9);
	buf[0] = 'x';
	BPLONG(buf+1, p.x);
	BPLONG(buf+5, p.y);
	bflush();
}
