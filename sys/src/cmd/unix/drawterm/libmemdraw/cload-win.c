#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

int
cloadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	return _cloadmemimage(i, r, data, ndata);
}
