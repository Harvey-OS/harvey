#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

#define RGB2K(r,g,b)	((156763*(r)+307758*(g)+59769*(b))>>19)

Point ZP = {0, 0};

Rectangle physgscreenr;

Memdata gscreendata;
Memimage *gscreen;

VGAscr vgascreen[1];

Cursor	arrow = {
	{ -1, -1 },
	{ 0xFF, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0C, 
	  0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04, 
	  0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8C, 0x04, 
	  0x92, 0x08, 0x91, 0x10, 0xA0, 0xA0, 0xC0, 0x40, 
	},
	{ 0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFC, 0x7F, 0xF0, 
	  0x7F, 0xE0, 0x7F, 0xE0, 0x7F, 0xF0, 0x7F, 0xF8, 
	  0x7F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFC, 0x73, 0xF8, 
	  0x61, 0xF0, 0x60, 0xE0, 0x40, 0x40, 0x00, 0x00, 
	},
};

int
screensize(int x, int y, int z, ulong chan)
{
	VGAscr *scr;

	memimageinit();
	scr = &vgascreen[0];

	/*
	 * BUG: need to check if any xalloc'ed memory needs to
	 * be given back if aperture is set.
	 */
	if(scr->aperture == 0){
		int width = (x*z)/BI2WD;

		gscreendata.bdata = xalloc(width*BY2WD*y);
		if(gscreendata.bdata == 0)
			error("screensize: vga soft memory");
/*		memset(gscreendata.bdata, 0x72, width*BY2WD*y);	/* not really black */
		scr->useflush = 1;
		scr->aperture = 0xA0000;
		scr->apsize = 1<<16;
	}
	else
		gscreendata.bdata = KADDR(scr->aperture);

	if(gscreen)
		freememimage(gscreen);

	gscreen = allocmemimaged(Rect(0,0,x,y), chan, &gscreendata);
	vgaimageinit(chan);
	if(gscreen == nil)
		return -1;

	if(scr->dev && scr->dev->flush)
		scr->useflush = 1;

	scr->palettedepth = 6;	/* default */
	scr->gscreendata = &gscreendata;
	scr->memdefont = getmemdefont();
	scr->gscreen = gscreen;

	physgscreenr = gscreen->r;

	drawcmap();
	return 0;
}

int
screenaperture(int size, int align)
{
	VGAscr *scr;
	ulong aperture;

	scr = &vgascreen[0];

	if(size == 0){
		if(scr->aperture && scr->isupamem)
			upafree(scr->aperture, scr->apsize);
		scr->aperture = 0;
		scr->isupamem = 0;
		return 0;
	}
	if(scr->dev && scr->dev->linear){
		aperture = scr->dev->linear(scr, &size, &align);
		if(aperture == 0)
			return 1;
	}else{
		aperture = upamalloc(0, size, align);
		if(aperture == 0)
			return 1;

		if(scr->aperture && scr->isupamem)
			upafree(scr->aperture, scr->apsize);
		scr->isupamem = 1;
	}

	scr->aperture = aperture;
	scr->apsize = size;

	return 0;
}

uchar*
attachscreen(Rectangle* r, ulong* chan, int* d, int* width, int *softscreen)
{
	VGAscr *scr;

	scr = &vgascreen[0];
	if(scr->gscreen == nil || scr->gscreendata == nil)
		return nil;

	*r = scr->gscreen->clipr;
	*chan = scr->gscreen->chan;
	*d = scr->gscreen->depth;
	*width = scr->gscreen->width;
	*softscreen = scr->useflush;

	return scr->gscreendata->bdata;
}

/*
 * It would be fair to say that this doesn't work for >8-bit screens.
 */
void
flushmemscreen(Rectangle r)
{
	VGAscr *scr;
	uchar *sp, *disp, *sdisp, *edisp;
	int y, len, incs, off, page;

	scr = &vgascreen[0];
	if(scr->dev && scr->dev->flush){
		scr->dev->flush(scr, r);
		return;
	}
	if(scr->gscreen == nil || scr->useflush == 0)
		return;
	if(scr->dev == nil || scr->dev->page == nil)
		return;

	if(rectclip(&r, scr->gscreen->r) == 0)
		return;

	incs = scr->gscreen->width * BY2WD;

	switch(scr->gscreen->depth){
	default:
		len = 0;
		panic("flushmemscreen: depth\n");
		break;
	case 8:
		len = Dx(r);
		break;
	}
	if(len < 1)
		return;

	off = r.min.y*scr->gscreen->width*BY2WD+(r.min.x*scr->gscreen->depth)/8;
	page = off/scr->apsize;
	off %= scr->apsize;
	disp = KADDR(scr->aperture);
	sdisp = disp+off;
	edisp = disp+scr->apsize;

	off = r.min.y*scr->gscreen->width*BY2WD+(r.min.x*scr->gscreen->depth)/8;

	sp = scr->gscreendata->bdata + off;

	scr->dev->page(scr, page);
	for(y = r.min.y; y < r.max.y; y++) {
		if(sdisp + incs < edisp) {
			memmove(sdisp, sp, len);
			sp += incs;
			sdisp += incs;
		}
		else {
			off = edisp - sdisp;
			page++;
			if(off <= len){
				if(off > 0)
					memmove(sdisp, sp, off);
				scr->dev->page(scr, page);
				if(len - off > 0)
					memmove(disp, sp+off, len - off);
			}
			else {
				memmove(sdisp, sp, len);
				scr->dev->page(scr, page);
			}
			sp += incs;
			sdisp += incs - scr->apsize;
		}
	}
}

void
getcolor(ulong p, ulong* pr, ulong* pg, ulong* pb)
{
	VGAscr *scr;
	ulong x;

	scr = &vgascreen[0];
	if(scr->gscreen == nil)
		return;

	switch(scr->gscreen->depth){
	default:
		x = 0x0F;
		break;
	case 8:
		x = 0xFF;
		break;
	}
	p &= x;

	lock(&cursor);
	*pr = scr->colormap[p][0];
	*pg = scr->colormap[p][1];
	*pb = scr->colormap[p][2];
	unlock(&cursor);
}

int
setpalette(ulong p, ulong r, ulong g, ulong b)
{
	VGAscr *scr;
	int d;

	scr = &vgascreen[0];
	d = scr->palettedepth;

	lock(&cursor);
	scr->colormap[p][0] = r;
	scr->colormap[p][1] = g;
	scr->colormap[p][2] = b;
	vgao(PaddrW, p);
	vgao(Pdata, r>>(32-d));
	vgao(Pdata, g>>(32-d));
	vgao(Pdata, b>>(32-d));
	unlock(&cursor);

	return ~0;
}

/*
 * On some video cards (e.g. Mach64), the palette is used as the 
 * DAC registers for >8-bit modes.  We don't want to set them when the user
 * is trying to set a colormap and the card is in one of these modes.
 */
int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	VGAscr *scr;
	int x;

	scr = &vgascreen[0];
	if(scr->gscreen == nil)
		return 0;

	switch(scr->gscreen->depth){
	case 1:
	case 2:
	case 4:
		x = 0x0F;
		break;
	case 8:
		x = 0xFF;
		break;
	default:
		return 0;
	}
	p &= x;

	return setpalette(p, r, g, b);
}

int
cursoron(int dolock)
{
	VGAscr *scr;
	int v;

	scr = &vgascreen[0];
	if(scr->cur == nil || scr->cur->move == nil)
		return 0;

	if(dolock)
		lock(&cursor);
	v = scr->cur->move(scr, mousexy());
	if(dolock)
		unlock(&cursor);

	return v;
}

void
cursoroff(int)
{
}

void
setcursor(Cursor* curs)
{
	VGAscr *scr;

	scr = &vgascreen[0];
	if(scr->cur == nil || scr->cur->load == nil)
		return;

	scr->cur->load(scr, curs);
}

int hwaccel = 1;
int hwblank = 0;	/* turned on by drivers that are known good */
int panning = 0;

int
hwdraw(Memdrawparam *par)
{
	VGAscr *scr;
	Memimage *dst, *src;
	int m;

	if(hwaccel == 0)
		return 0;

	dst = par->dst;
	scr = &vgascreen[0];
	if(dst == nil || dst->data == nil)
		return 0;

	if(dst->data->bdata != gscreendata.bdata)
		return 0;

	if(scr->fill==nil && scr->scroll==nil)
		return 0;

	/*
	 * If we have an opaque mask and source is one opaque
	 * pixel we can convert to the destination format and just
	 * replicate with memset.
	 */
	m = Simplesrc|Simplemask|Fullmask;
	if(scr->fill && (par->state&m)==m && ((par->srgba&0xFF) == 0xFF))
		return scr->fill(scr, par->r, par->sdval);

	/*
	 * If no source alpha, an opaque mask, we can just copy the
	 * source onto the destination.  If the channels are the same and
	 * the source is not replicated, memmove suffices.
	 */
	src = par->src;
	if(scr->scroll && src->data->bdata==dst->data->bdata && !(src->flags&Falpha)
	&& (par->state&(Simplemask|Fullmask))==(Simplemask|Fullmask)){
		return scr->scroll(scr, par->r, par->sr);
	}

	return 0;	
}

void
blankscreen(int blank)
{
	VGAscr *scr;

	scr = &vgascreen[0];
	if(hwblank){
		if(scr->blank)
			scr->blank(scr, blank);
		else
			vgablank(scr, blank);
	}
}
