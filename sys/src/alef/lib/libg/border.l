#include <alef.h>
#include <graphics.h>

void
border(Bitmap *l, Rectangle r, int i, Fcode c)
{
	bitblt(l, r.min, l, Rect(r.min.x, r.min.y, r.max.x, r.min.y+i), c);
	bitblt(l, Pt(r.min.x, r.max.y-i),
		l, Rect(r.min.x, r.max.y-i, r.max.x, r.max.y), c);
	bitblt(l, Pt(r.min.x, r.min.y+i),
		l, Rect(r.min.x, r.min.y+i, r.min.x+i, r.max.y-i), c);
	bitblt(l, Pt(r.max.x-i, r.min.y+i),
		l, Rect(r.max.x-i, r.min.y+i, r.max.x, r.max.y-i), c);
}
