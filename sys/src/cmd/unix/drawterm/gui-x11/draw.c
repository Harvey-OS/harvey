#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include "xmem.h"

void xfillcolor(Memimage*, Rectangle, ulong);
static int xdraw(Memdrawparam*);

int	xgcfillcolor = 0;
int	xgcfillcolor0 = 0;
int	xgczeropm = 0;
int	xgczeropm0 = 0;
int	xgcsimplecolor = 0;
int	xgcsimplecolor0 = 0;
int	xgcsimplepm = 0; 
int	xgcsimplepm0 = 0;
int	xgcreplsrctile = 0;
int	xgcreplsrctile0 = 0;

void
memimageinit(void)
{
	static int didinit = 0;
	
	if(didinit)
		return;

	didinit = 1;
	_memimageinit();
	
	xfillcolor(memblack, memblack->r, 0);
	xfillcolor(memwhite, memwhite->r, 1);
}

void
memimagedraw(Memimage *dst, Rectangle r, Memimage *src, Point sp, Memimage *mask, Point mp, int op)
{
	Memdrawparam *par;
	
	if((par = _memimagedrawsetup(dst, r, src, sp, mask, mp, op)) == nil)
		return;
	_memimagedraw(par);
	if(!xdraw(par))
		putXdata(dst, par->r);
}

void
xfillcolor(Memimage *m, Rectangle r, ulong v)
{
	GC gc;
	Xmem *dxm;

	dxm = m->X;
	assert(dxm != nil);
	r = rectsubpt(r, m->r.min);
		
	if(m->chan == GREY1){
		gc = xgcfill0;
		if(xgcfillcolor0 != v){
			XSetForeground(xdisplay, gc, v);
			xgcfillcolor0 = v;
		}
	}else{
		if(m->chan == CMAP8 && xtblbit)
			v = plan9tox11[v];
				
		gc = xgcfill;
		if(xgcfillcolor != v){
			XSetForeground(xdisplay, gc, v);
			xgcfillcolor = v;
		}
	}
	XFillRectangle(xdisplay, dxm->pmid, gc, r.min.x, r.min.y, Dx(r), Dy(r));
}

static int
xdraw(Memdrawparam *par)
{
	int dy, dx;
	unsigned m;
	Memimage *src, *dst, *mask;
	Xmem *dxm, *sxm, *mxm;
	GC gc;
	Rectangle r, sr, mr;
	ulong sdval;

	dx = Dx(par->r);
	dy = Dy(par->r);
	src = par->src;
	dst = par->dst;
	mask = par->mask;
	r = par->r;
	sr = par->sr;
	mr = par->mr;
	sdval = par->sdval;

return 0;
	if((dxm = dst->X) == nil)
		return 0;

	/*
	 * If we have an opaque mask and source is one opaque pixel we can convert to the
	 * destination format and just XFillRectangle.
	 */
	m = Simplesrc|Simplemask|Fullmask;
	if((par->state&m)==m){
		xfillcolor(dst, r, sdval);
		dirtyXdata(dst, par->r);
		return 1;
	}

	/*
	 * If no source alpha, an opaque mask, we can just copy the
	 * source onto the destination.  If the channels are the same and
	 * the source is not replicated, XCopyArea suffices.
	 */
	m = Simplemask|Fullmask;
	if((par->state&(m|Replsrc))==m && src->chan == dst->chan && src->X){
		sxm = src->X;
		r = rectsubpt(r, dst->r.min);		
		sr = rectsubpt(sr, src->r.min);
		if(dst->chan == GREY1)
			gc = xgccopy0;
		else
			gc = xgccopy;
		XCopyArea(xdisplay, sxm->pmid, dxm->pmid, gc, 
			sr.min.x, sr.min.y, dx, dy, r.min.x, r.min.y);
		dirtyXdata(dst, par->r);
		return 1;
	}
	
	/*
	 * If no source alpha, a 1-bit mask, and a simple source
	 * we can just copy through the mask onto the destination.
	 */
	if(dst->X && mask->X && !(mask->flags&Frepl)
	&& mask->chan == GREY1 && (par->state&Simplesrc)){
		Point p;

		mxm = mask->X;
		r = rectsubpt(r, dst->r.min);		
		mr = rectsubpt(mr, mask->r.min);
		p = subpt(r.min, mr.min);
		if(dst->chan == GREY1){
			gc = xgcsimplesrc0;
			if(xgcsimplecolor0 != sdval){
				XSetForeground(xdisplay, gc, sdval);
				xgcsimplecolor0 = sdval;
			}
			if(xgcsimplepm0 != mxm->pmid){
				XSetStipple(xdisplay, gc, mxm->pmid);
				xgcsimplepm0 = mxm->pmid;
			}
		}else{
		/* somehow this doesn't work on rob's mac 
			gc = xgcsimplesrc;
			if(dst->chan == CMAP8 && xtblbit)
				sdval = plan9tox11[sdval];
				
			if(xgcsimplecolor != sdval){
				XSetForeground(xdisplay, gc, sdval);
				xgcsimplecolor = sdval;
			}
			if(xgcsimplepm != mxm->pmid){
				XSetStipple(xdisplay, gc, mxm->pmid);
				xgcsimplepm = mxm->pmid;
			}
		*/
			return 0;
		}
		XSetTSOrigin(xdisplay, gc, p.x, p.y);
		XFillRectangle(xdisplay, dxm->pmid, gc, r.min.x, r.min.y, dx, dy);
		dirtyXdata(dst, par->r);
		return 1;
	}
	return 0;
}

ulong
pixelbits(Memimage *m, Point p)
{
	if(m->X)
		getXdata(m, Rect(p.x, p.y, p.x+1, p.y+1));
	return _pixelbits(m, p);
}
