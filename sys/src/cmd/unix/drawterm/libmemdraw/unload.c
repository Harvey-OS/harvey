#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

int
unloadmemimage(Memimage *i, Rectangle r, uchar *data, int ndata)
{
	int y, l;
	uchar *q;

	if(!rectinrect(r, i->r))
		return -1;
	l = bytesperline(r, i->depth);
	if(ndata < l*Dy(r))
		return -1;
	ndata = l*Dy(r);
	q = byteaddr(i, r.min);
	for(y=r.min.y; y<r.max.y; y++){
		memmove(data, q, l);
		q += i->width*sizeof(ulong);
		data += l;
	}
	return ndata;
}
