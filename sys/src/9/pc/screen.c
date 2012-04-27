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

int didswcursorinit;

static void *softscreen;

int
screensize(int x, int y, int z, ulong chan)
{
	VGAscr *scr;
	void *oldsoft;

	lock(&vgascreenlock);
	if(waserror()){
		unlock(&vgascreenlock);
		nexterror();
	}

	memimageinit();
	scr = &vgascreen[0];
	oldsoft = softscreen;

	if(scr->paddr == 0){
		int width = (x*z)/BI2WD;
		void *p;

		p = xalloc(width*BY2WD*y);
		if(p == nil)
			error("no memory for vga soft screen");
		gscreendata.bdata = softscreen = p;
		if(scr->dev && scr->dev->page){
			scr->vaddr = KADDR(VGAMEM());
			scr->apsize = 1<<16;
		}
		scr->useflush = 1;
	}
	else{
		gscreendata.bdata = scr->vaddr;
		scr->useflush = scr->dev && scr->dev->flush;
	}

	scr->gscreen = nil;
	if(gscreen)
		freememimage(gscreen);
	gscreen = allocmemimaged(Rect(0,0,x,y), chan, &gscreendata);
	if(gscreen == nil)
		error("no memory for vga memimage");
	vgaimageinit(chan);

	scr->palettedepth = 6;	/* default */
	scr->gscreendata = &gscreendata;
	scr->memdefont = getmemdefont();
	scr->gscreen = gscreen;

	physgscreenr = gscreen->r;
	unlock(&vgascreenlock);
	poperror();
	if(oldsoft)
		xfree(oldsoft);

	memimagedraw(gscreen, gscreen->r, memblack, ZP, nil, ZP, S);
	flushmemscreen(gscreen->r);

	if(didswcursorinit)
		swcursorinit();
	drawcmap();
	return 0;
}

int
screenaperture(int size, int align)
{
	VGAscr *scr;

	scr = &vgascreen[0];

	if(scr->paddr)	/* set up during enable */
		return 0;

	if(size == 0)
		return 0;

	if(scr->dev && scr->dev->linear){
		scr->dev->linear(scr, size, align);
		return 0;
	}

	/*
	 * Need to allocate some physical address space.
	 * The driver will tell the card to use it.
	 */
	size = PGROUND(size);
	scr->paddr = upaalloc(size, align);
	if(scr->paddr == 0)
		return -1;
	scr->vaddr = vmap(scr->paddr, size);
	if(scr->vaddr == nil)
		return -1;
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
	disp = scr->vaddr;
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
	Memimage *dst, *src, *mask;
	int m;

	if(hwaccel == 0)
		return 0;

	scr = &vgascreen[0];
	if((dst=par->dst) == nil || dst->data == nil)
		return 0;
	if((src=par->src) == nil || src->data == nil)
		return 0;
	if((mask=par->mask) == nil || mask->data == nil)
		return 0;

	if(scr->cur == &swcursor){
		/*
		 * always calling swcursorhide here doesn't cure
		 * leaving cursor tracks nor failing to refresh menus
		 * with the latest libmemdraw/draw.c.
		 */
		if(dst->data->bdata == gscreendata.bdata)
			swcursoravoid(par->r);
		if(src->data->bdata == gscreendata.bdata)
			swcursoravoid(par->sr);
		if(mask->data->bdata == gscreendata.bdata)
			swcursoravoid(par->mr);
	}
	
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
	if(scr->fill
	&& (par->state&m)==m
	&& ((par->srgba&0xFF) == 0xFF)
	&& (par->op&S) == S)
		return scr->fill(scr, par->r, par->sdval);

	/*
	 * If no source alpha, an opaque mask, we can just copy the
	 * source onto the destination.  If the channels are the same and
	 * the source is not replicated, memmove suffices.
	 */
	m = Simplemask|Fullmask;
	if(scr->scroll
	&& src->data->bdata==dst->data->bdata
	&& !(src->flags&Falpha)
	&& (par->state&m)==m
	&& (par->op&S) == S)
		return scr->scroll(scr, par->r, par->sr);

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

void
vgalinearpciid(VGAscr *scr, int vid, int did)
{
	Pcidev *p;

	p = nil;
	while((p = pcimatch(p, vid, 0)) != nil){
		if(p->ccrb != 3)	/* video card */
			continue;
		if(did != 0 && p->did != did)
			continue;
		break;
	}
	if(p == nil)
		error("pci video card not found");

	scr->pci = p;
	vgalinearpci(scr);
}

void
vgalinearpci(VGAscr *scr)
{
	ulong paddr;
	int i, size, best;
	Pcidev *p;
	
	p = scr->pci;
	if(p == nil)
		return;

	/*
	 * Scan for largest memory region on card.
	 * Some S3 cards (e.g. Savage) have enormous
	 * mmio regions (but even larger frame buffers).
	 * Some 3dfx cards (e.g., Voodoo3) have mmio
	 * buffers the same size as the frame buffer,
	 * but only the frame buffer is marked as
	 * prefetchable (bar&8).  If a card doesn't fit
	 * into these heuristics, its driver will have to
	 * call vgalinearaddr directly.
	 */
	best = -1;
	for(i=0; i<nelem(p->mem); i++){
		if(p->mem[i].bar&1)	/* not memory */
			continue;
		if(p->mem[i].size < 640*480)	/* not big enough */
			continue;
		if(best==-1 
		|| p->mem[i].size > p->mem[best].size 
		|| (p->mem[i].size == p->mem[best].size 
		  && (p->mem[i].bar&8)
		  && !(p->mem[best].bar&8)))
			best = i;
	}
	if(best >= 0){
		paddr = p->mem[best].bar & ~0x0F;
		size = p->mem[best].size;
		vgalinearaddr(scr, paddr, size);
		return;
	}
	error("no video memory found on pci card");
}

void
vgalinearaddr(VGAscr *scr, ulong paddr, int size)
{
	int x, nsize;
	ulong npaddr;

	/*
	 * new approach.  instead of trying to resize this
	 * later, let's assume that we can just allocate the
	 * entire window to start with.
	 */

	if(scr->paddr == paddr && size <= scr->apsize)
		return;

	if(scr->paddr){
		/*
		 * could call vunmap and vmap,
		 * but worried about dangling pointers in devdraw
		 */
		error("cannot grow vga frame buffer");
	}
	
	/* round to page boundary, just in case */
	x = paddr&(BY2PG-1);
	npaddr = paddr-x;
	nsize = PGROUND(size+x);

	/*
	 * Don't bother trying to map more than 4000x4000x32 = 64MB.
	 * We only have a 256MB window.
	 */
	if(nsize > 64*MB)
		nsize = 64*MB;
	scr->vaddr = vmap(npaddr, nsize);
	if(scr->vaddr == 0)
		error("cannot allocate vga frame buffer");
	scr->vaddr = (char*)scr->vaddr+x;
	scr->paddr = paddr;
	scr->apsize = nsize;
	/* let mtrr harmlessly fail on old CPUs, e.g., P54C */
	if(!waserror()){
		mtrr(npaddr, nsize, "wc");
		poperror();
	}
}


/*
 * Software cursor. 
 */
int	swvisible;	/* is the cursor visible? */
int	swenabled;	/* is the cursor supposed to be on the screen? */
Memimage*	swback;	/* screen under cursor */
Memimage*	swimg;	/* cursor image */
Memimage*	swmask;	/* cursor mask */
Memimage*	swimg1;
Memimage*	swmask1;

Point	swoffset;
Rectangle	swrect;	/* screen rectangle in swback */
Point	swpt;	/* desired cursor location */
Point	swvispt;	/* actual cursor location */
int	swvers;	/* incremented each time cursor image changes */
int	swvisvers;	/* the version on the screen */

/*
 * called with drawlock locked for us, most of the time.
 * kernel prints at inopportune times might mean we don't
 * hold the lock, but memimagedraw is now reentrant so
 * that should be okay: worst case we get cursor droppings.
 */
void
swcursorhide(void)
{
	if(swvisible == 0)
		return;
	if(swback == nil)
		return;
	swvisible = 0;
	memimagedraw(gscreen, swrect, swback, ZP, memopaque, ZP, S);
	flushmemscreen(swrect);
}

void
swcursoravoid(Rectangle r)
{
	if(swvisible && rectXrect(r, swrect))
		swcursorhide();
}

void
swcursordraw(void)
{
	if(swvisible)
		return;
	if(swenabled == 0)
		return;
	if(swback == nil || swimg1 == nil || swmask1 == nil)
		return;
	assert(!canqlock(&drawlock));
	swvispt = swpt;
	swvisvers = swvers;
	swrect = rectaddpt(Rect(0,0,16,16), swvispt);
	memimagedraw(swback, swback->r, gscreen, swpt, memopaque, ZP, S);
	memimagedraw(gscreen, swrect, swimg1, ZP, swmask1, ZP, SoverD);
	flushmemscreen(swrect);
	swvisible = 1;
}

/*
 * Need to lock drawlock for ourselves.
 */
void
swenable(VGAscr*)
{
	swenabled = 1;
	if(canqlock(&drawlock)){
		swcursordraw();
		qunlock(&drawlock);
	}
}

void
swdisable(VGAscr*)
{
	swenabled = 0;
	if(canqlock(&drawlock)){
		swcursorhide();
		qunlock(&drawlock);
	}
}

void
swload(VGAscr*, Cursor *curs)
{
	uchar *ip, *mp;
	int i, j, set, clr;

	if(!swimg || !swmask || !swimg1 || !swmask1)
		return;
	/*
	 * Build cursor image and mask.
	 * Image is just the usual cursor image
	 * but mask is a transparent alpha mask.
	 * 
	 * The 16x16x8 memimages do not have
	 * padding at the end of their scan lines.
	 */
	ip = byteaddr(swimg, ZP);
	mp = byteaddr(swmask, ZP);
	for(i=0; i<32; i++){
		set = curs->set[i];
		clr = curs->clr[i];
		for(j=0x80; j; j>>=1){
			*ip++ = set&j ? 0x00 : 0xFF;
			*mp++ = (clr|set)&j ? 0xFF : 0x00;
		}
	}
	swoffset = curs->offset;
	swvers++;
	memimagedraw(swimg1, swimg1->r, swimg, ZP, memopaque, ZP, S);
	memimagedraw(swmask1, swmask1->r, swmask, ZP, memopaque, ZP, S);
}

int
swmove(VGAscr*, Point p)
{
	swpt = addpt(p, swoffset);
	return 0;
}

void
swcursorclock(void)
{
	int x;

	if(!swenabled)
		return;
	if(swvisible && eqpt(swpt, swvispt) && swvers==swvisvers)
		return;

	x = splhi();
	if(swenabled)
	if(!swvisible || !eqpt(swpt, swvispt) || swvers!=swvisvers)
	if(canqlock(&drawlock)){
		swcursorhide();
		swcursordraw();
		qunlock(&drawlock);
	}
	splx(x);
}

void
swcursorinit(void)
{
	static int init, warned;
	VGAscr *scr;

	didswcursorinit = 1;
	if(!init){
		init = 1;
		addclock0link(swcursorclock, 10);
	}
	scr = &vgascreen[0];
	if(scr==nil || scr->gscreen==nil)
		return;

	if(scr->dev == nil || scr->dev->linear == nil){
		if(!warned){
			print("cannot use software cursor on non-linear vga screen\n");
			warned = 1;
		}
		return;
	}

	if(swback){
		freememimage(swback);
		freememimage(swmask);
		freememimage(swmask1);
		freememimage(swimg);
		freememimage(swimg1);
	}

	swback = allocmemimage(Rect(0,0,32,32), gscreen->chan);
	swmask = allocmemimage(Rect(0,0,16,16), GREY8);
	swmask1 = allocmemimage(Rect(0,0,16,16), GREY1);
	swimg = allocmemimage(Rect(0,0,16,16), GREY8);
	swimg1 = allocmemimage(Rect(0,0,16,16), GREY1);
	if(swback==nil || swmask==nil || swmask1==nil || swimg==nil || swimg1 == nil){
		print("software cursor: allocmemimage fails");
		return;
	}

	memfillcolor(swmask, DOpaque);
	memfillcolor(swmask1, DOpaque);
	memfillcolor(swimg, DBlack);
	memfillcolor(swimg1, DBlack);
}

VGAcur swcursor =
{
	"soft",
	swenable,
	swdisable,
	swload,
	swmove,
};

