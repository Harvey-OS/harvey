#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

int
loadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	return _loadmemimage(i, r, data, ndata);
}
