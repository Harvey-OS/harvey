#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"
#include "xmem.h"

int
cloadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	int n;

	n = _cloadmemimage(i, r, data, ndata);
	if(n > 0 && i->X)
		putXdata(i, r);
	return n;
}
