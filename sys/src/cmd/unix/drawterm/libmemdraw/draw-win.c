#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

void
memimagedraw(Memimage *dst, Rectangle r, Memimage *src, Point p0, Memimage *mask, Point p1)
{
	_memimagedraw(_memimagedrawsetup(dst, r, src, p0, mask, p1));
}

ulong
pixelbits(Memimage *m, Point p)
{
	return _pixelbits(m, p);
}

void
memimageinit(void)
{
	_memimageinit();
}
