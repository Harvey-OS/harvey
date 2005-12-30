#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

void
memimagedraw(Memimage *dst, Rectangle r, Memimage *src, Point sp, Memimage *mask, Point mp, int op)
{
	_memimagedraw(_memimagedrawsetup(dst, r, src, sp, mask, mp, op));
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
