#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

void
mempoly(Memimage *dst, Point *vert, int nvert, int end0, int end1, int radius, Memimage *src, Point sp)
{
	int i, e0, e1;
	Point d;
	Point tp, tp_1;

	if(nvert < 2)
		return;

	/* use temp tp to avoid unaligned volatile warning on digital unix */
	tp = vert[0];
	d = subpt(sp, tp);
	for(i=1; i<nvert; i++){
		e0 = e1 = Enddisc;
		if(i == 1)
			e0 = end0;
		if(i == nvert-1)
			e1 = end1;
		tp = vert[i];
		tp_1 = vert[i-1];
		memline(dst, tp_1, tp, e0, e1, radius, src, addpt(d, tp_1));
	/*	memline(dst, vert[i-1], vert[i], e0, e1, radius, src, addpt(d, vert[i-1])); */
	}
}
