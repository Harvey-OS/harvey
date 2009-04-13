#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

Memimage*
allocmemimage(Rectangle r, ulong chan)
{
	return _allocmemimage(r, chan);
}

void
freememimage(Memimage *i)
{
	_freememimage(i);
}

void
memfillcolor(Memimage *i, ulong val)
{
	_memfillcolor(i, val);
}

