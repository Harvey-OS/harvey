#include <alef.h>
#include <libg.h>

void
cursorset(Point p)
{
	byte *buf;

	buf = bneed(9);
	buf[0] = 'x';
	BPLONG(buf+1, p.x);
	BPLONG(buf+5, p.y);
	bflush();
}
