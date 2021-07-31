#include <u.h>
#include <libc.h>
#include <draw.h>

static void
draw1(Image *dst, Rectangle *r, Image *src, Point *p0, Image *mask, Point *p1)
{
	uchar *a;

	a = bufimage(dst->display, 1+4+4+4+4*4+2*4+2*4+(dst->display->_isnewdisplay?1:0));
	if(a == 0)
		return;
	if(src == nil)
		src = dst->display->black;
	if(mask == nil)
		mask = dst->display->opaque;
	a[0] = 'd';
	BPLONG(a+1, dst->id);
	BPLONG(a+5, src->id);
	BPLONG(a+9, mask->id);
	BPLONG(a+13, r->min.x);
	BPLONG(a+17, r->min.y);
	BPLONG(a+21, r->max.x);
	BPLONG(a+25, r->max.y);
	BPLONG(a+29, p0->x);
	BPLONG(a+33, p0->y);
	BPLONG(a+37, p1->x);
	BPLONG(a+41, p1->y);
	if(dst->display->_isnewdisplay)
		a[41] = SoverD;
}

void
draw(Image *dst, Rectangle r, Image *src, Image *mask, Point p1)
{
	draw1(dst, &r, src, &p1, mask, &p1);
}

void
gendraw(Image *dst, Rectangle r, Image *src, Point p0, Image *mask, Point p1)
{
	draw1(dst, &r, src, &p0, mask, &p1);
}
