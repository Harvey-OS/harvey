#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

/*
 * Wordaddr is deprecated.
 */
ulong*
wordaddr(Memimage *i, Point p)
{
	return (ulong*) ((uintptr)byteaddr(i, p) & ~(sizeof(ulong)-1));
}

uchar*
byteaddr(Memimage *i, Point p)
{
	uchar *a;

	a = i->data->bdata+i->zero+sizeof(ulong)*p.y*i->width;

	if(i->depth < 8){
		/*
		 * We need to always round down,
		 * but C rounds toward zero.
		 */
		int np;
		np = 8/i->depth;
		if(p.x < 0)
			return a+(p.x-np+1)/np;
		else
			return a+p.x/np;
	}
	else
		return a+p.x*(i->depth/8);
}
