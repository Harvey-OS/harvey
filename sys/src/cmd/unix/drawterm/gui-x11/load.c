#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include "xmem.h"

int
loadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	int n;

	n = _loadmemimage(i, r, data, ndata);
	if(n > 0 && i->X)
		putXdata(i, r);
	return n;
}
