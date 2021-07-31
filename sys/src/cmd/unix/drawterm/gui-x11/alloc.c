#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include "xmem.h"

/* perfect approximation to NTSC = .299r+.587g+.114b when 0 ≤ r,g,b < 256 */
#define RGB2K(r,g,b)	((156763*(r)+307758*(g)+59769*(b))>>19)

Memimage*
xallocmemimage(Rectangle r, ulong chan, int pmid)
{
	Memimage *m;
	Xmem *xm;
	XImage *xi;
	int offset;
	int d;
	
	m = _allocmemimage(r, chan);
	if(chan != GREY1 && chan != xscreenchan)
		return m;

	d = m->depth;
	xm = mallocz(sizeof(Xmem), 1);
	if(pmid != PMundef)
		xm->pmid = pmid;
	else
		xm->pmid = XCreatePixmap(xdisplay, xscreenid, Dx(r), Dy(r), (d==32) ? 24 : d);
		
	if(m->depth == 24)
		offset = r.min.x&(4-1);
	else
		offset = r.min.x&(31/m->depth);
	r.min.x -= offset;
	
	assert(wordsperline(r, m->depth) <= m->width);

	xi = XCreateImage(xdisplay, xvis, m->depth==32?24:m->depth, ZPixmap, 0,
		(char*)m->data->bdata, Dx(r), Dy(r), 32, m->width*sizeof(ulong));
	
	if(xi == nil){
		_freememimage(m);
		return nil;
	}

	xm->xi = xi;
	xm->pc = getcallerpc(&r);
	xm->r = r;
	
	/*
	 * Set the parameters of the XImage so its memory looks exactly like a
	 * Memimage, so we can call _memimagedraw on the same data.  All frame
	 * buffers we've seen, and Plan 9's graphics code, require big-endian
	 * bits within bytes, but little endian byte order within pixels.
	 */
	xi->bitmap_unit = m->depth < 8 || m->depth == 24 ? 8 : m->depth;
	xi->byte_order = LSBFirst;
	xi->bitmap_bit_order = MSBFirst;
	xi->bitmap_pad = 32;
	xm->r = Rect(0,0,0,0);
	XInitImage(xi);
	XFlush(xdisplay);

	m->X = xm;
	return m;
}

Memimage*
allocmemimage(Rectangle r, ulong chan)
{
	return xallocmemimage(r, chan, PMundef);
}

void
freememimage(Memimage *m)
{
	Xmem *xm;
	
	if(m == nil)
		return;
		
	if(m->data->ref == 1){
		if((xm = m->X) != nil){
			if(xm->xi){
				xm->xi->data = nil;
				XFree(xm->xi);
			}
			XFreePixmap(xdisplay, xm->pmid);
			free(xm);
			m->X = nil;
		}
	}
	_freememimage(m);
}

void
memfillcolor(Memimage *m, ulong val)
{
	_memfillcolor(m, val);
	if(m->X){
		if((val & 0xFF) == 0xFF)
			xfillcolor(m, m->r, _rgbatoimg(m, val));
		else
			putXdata(m, m->r);
	}
}

static void
addrect(Rectangle *rp, Rectangle r)
{
	if(rp->min.x >= rp->max.x)
		*rp = r;
	else
		combinerect(rp, r);
}

XImage*
getXdata(Memimage *m, Rectangle r)
{
	uchar *p;
	int x, y;
	Xmem *xm;
	Point xdelta, delta;
	Point tp;

 	xm = m->X;
 	if(xm == nil)
 		return nil;
 
	assert(xm != nil && xm->xi != nil);
	
 	if(xm->dirty == 0)
 		return xm->xi;
 		
 	r = xm->dirtyr;
	if(Dx(r)==0 || Dy(r)==0)
		return xm->xi;

	delta = subpt(r.min, m->r.min);
	tp = xm->r.min;	/* avoid unaligned access on digital unix */
	xdelta = subpt(r.min, tp);
	
	XGetSubImage(xdisplay, xm->pmid, delta.x, delta.y, Dx(r), Dy(r),
		AllPlanes, ZPixmap, xm->xi, xdelta.x, xdelta.y);
		
	if(xtblbit && m->chan == CMAP8)
		for(y=r.min.y; y<r.max.y; y++)
			for(x=r.min.x, p=byteaddr(m, Pt(x,y)); x<r.max.x; x++, p++)
				*p = x11toplan9[*p];
				
	xm->dirty = 0;
	xm->dirtyr = Rect(0,0,0,0);
	return xm->xi;
}

void
putXdata(Memimage *m, Rectangle r)
{
	Xmem *xm;
	XImage *xi;
	GC g;
	Point xdelta, delta;
	Point tp;
	int x, y;
	uchar *p;

	xm = m->X;
	if(xm == nil)
		return;
		
	assert(xm != nil);
	assert(xm->xi != nil);

	xi = xm->xi;

	g = (m->chan == GREY1) ? xgccopy0 : xgccopy;

	delta = subpt(r.min, m->r.min);
	tp = xm->r.min;	/* avoid unaligned access on digital unix */
	xdelta = subpt(r.min, tp);
	
	if(xtblbit && m->chan == CMAP8)
		for(y=r.min.y; y<r.max.y; y++)
			for(x=r.min.x, p=byteaddr(m, Pt(x,y)); x<r.max.x; x++, p++)
				*p = plan9tox11[*p];
	
	XPutImage(xdisplay, xm->pmid, g, xi, xdelta.x, xdelta.y, delta.x, delta.y, Dx(r), Dy(r));

	if(xtblbit && m->chan == CMAP8)
		for(y=r.min.y; y<r.max.y; y++)
			for(x=r.min.x, p=byteaddr(m, Pt(x,y)); x<r.max.x; x++, p++)
				*p = x11toplan9[*p];
}

void
dirtyXdata(Memimage *m, Rectangle r)
{
	Xmem *xm;
	
	if((xm = m->X) != nil){
		xm->dirty = 1;
		addrect(&xm->dirtyr, r);
	}
}
