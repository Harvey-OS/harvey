#include <u.h>
#include <libg.h>
#include <gnot.h>

ulong *
gaddr(GBitmap *b, Point p)
{
	return b->base+b->zero+(p.y*b->width)+(p.x>>(5-b->ldepth));
}

uchar *
gbaddr(GBitmap *b, Point p)
{
	return (uchar*)(b->base+b->zero+(p.y*b->width))+(p.x>>(3-b->ldepth));
}
