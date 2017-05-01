/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Simple coreboot framebuffer screen inspired by the omap code.
 * coreboot gives us an x by y screen with 24 bpp, 8 of rgb, always.
 * Note that in the coreboot case, this is architecture-independent code.
 */
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
#include <coreboot.h>
#include "screen.h"
#include "corebootscreen.h"

enum {
	Tabstop	= 4,		/* should be 8 */
	Scroll	= 8,		/* lines to scroll at one time */
};

extern struct sysinfo_t cbinfo;
int	drawdebug = 1;
Point	ZP = {0, 0};
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

static Memdata xgdata;

static Memimage xgscreen =
{
	{ 0, 0, 1024, 768 },	/* r */
	{ 0, 0, 1024, 768 },	/* clipr */
	24,			/* depth */
	3,			/* nchan */
	RGB24,			/* chan */
	nil,			/* cmap */
	&xgdata,		/* data */
	0,			/* zero */
	1024*768, 	/* width in words of a single scan line */
	0,			/* layer */
	0,			/* flags */
};

Memimage *gscreen;
static CorebootScreen Cbscreen;
static Memimage *conscol;
static Memimage *back;
static Corebootfb framebuf;
static Memsubfont *memdefont;

//static Lock screenlock;

static Point	curpos;
static int	h, w;
static int	landscape = 0;	/* screen orientation, default is 0: portrait */
//static uint32_t	*vscreen;	/* virtual screen */
static Rectangle window;

static struct VGAdev cbvgadev = {
	.name =  "coreboot",
/*

		void	(*enable)(VGAscr*);
		void	(*disable)(VGAscr*);
		void	(*page)(VGAscr*, int);
		void	(*linear)(VGAscr*, int, int);
		void	(*drawinit)(VGAscr*);
		int	(*fill)(VGAscr*, Rectangle, uint32_t);
		void	(*ovlctl)(VGAscr*, Chan*, void*, int);
		int	(*ovlwrite)(VGAscr*, void*, int, int64_t);
		void (*flush)(VGAscr*, Rectangle);
*/

};

struct VGAcur cbvgacur = {
	"coreboot",
/*
	void	(*enable)(VGAscr*);
	void	(*disable)(VGAscr*);
	void	(*load)(VGAscr*, Cursor*);
	int	(*move)(VGAscr*, Point);

	int	doespanning;
*/
};


static int scroll(VGAscr*, Rectangle, Rectangle);

// for devdraw
VGAscr vgascreen[1] = {
	{
	.dev = &cbvgadev,
	.cur = &cbvgacur,
	.gscreen = &xgscreen,
//	.memdefont = memdefont,
	.scroll = scroll,
	},
};

//static	void	corebootscreenputs(char *s, int n);
//static	uint32_t rep(uint32_t, int);
//static	void	screenputc(char *buf);
static	void	screenwin(void);

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

static void
lcdoff(void)
{
	print("LCD OFF\n");
	delay(20);			/* worst case for 1 frame, 50Hz */
}

static void
lcdon(int enable)
{
	print("%s\n", __func__);
	print("LCD ON\n");
	delay(10);
}

static void
lcdstop(void)
{
	print("%s\n", __func__);
	lcdoff();
	print("lcdstop\n");
}

static void
lcdinit(void)
{
	print("%s\n", __func__);
	print("lcdinit\n");
	lcdstop();

}

/* Paint the image data with blue pixels */
void
screentest(void)
{
	int i;

	print("%s\n", __func__);
	for (i = sizeof(framebuf.pixel)/sizeof(framebuf.pixel[0]) - 1; i >= 0; i--)
		framebuf.pixel[i] = 0x1f;			/* blue */
//	memset(framebuf.pixel, ~0, sizeof framebuf.pixel);	/* white */
}

void
screenpower(int on)
{
	print("%s\n", __func__);
	blankscreen(on == 0);
}

/*
 * called with drawlock locked for us, most of the time.
 * kernel prints at inopportune times might mean we don't
 * hold the lock, but memimagedraw is now reentrant so
 * that should be okay: worst case we get cursor droppings.
 */
void
swcursorhide(void)
{
	print("%s\n", __func__);
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
	print("%s\n", __func__);
	if(swvisible && rectXrect(r, swrect))
		swcursorhide();
}

void
swcursordraw(void)
{
	print("%s\n", __func__);
	if(swvisible)
		return;
	if(swenabled == 0)
		return;
	if(swback == nil || swimg1 == nil || swmask1 == nil)
		return;
//	assert(!canqlock(&drawlock));		// assertion fails on omap
	swvispt = swpt;
	swvisvers = swvers;
	swrect = rectaddpt(Rect(0,0,16,16), swvispt);
	memimagedraw(swback, swback->r, gscreen, swpt, memopaque, ZP, S);
	memimagedraw(gscreen, swrect, swimg1, ZP, swmask1, ZP, SoverD);
	flushmemscreen(swrect);
	swvisible = 1;
}

int
cursoron(int dolock)
{
	print("%s\n", __func__);
	if (dolock)
		lock(&Cbscreen.l);
	cursoroff(0);
	swcursordraw();
	if (dolock)
		unlock(&Cbscreen.l);
	return 0;
}

void
cursoroff(int dolock)
{
	if (dolock)
		lock(&Cbscreen.l);
	swcursorhide();
	if (dolock)
		unlock(&Cbscreen.l);
}

void
swload(Cursor *curs)
{
	print("%s\n", __func__);
	uint8_t *ip, *mp;
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
	memimagedraw(swimg1,  swimg1->r,  swimg,  ZP, memopaque, ZP, S);
	memimagedraw(swmask1, swmask1->r, swmask, ZP, memopaque, ZP, S);
}

/* called from devmouse */
void
setcursor(Cursor* curs)
{
	print("%s\n", __func__);
	cursoroff(1);
	Cbscreen.Cursor = *curs;
	swload(curs);
	cursoron(1);
}

int
swmove(Point p)
{
	print("%s\n", __func__);
	swpt = addpt(p, swoffset);
	return 0;
}

void
swcursorclock(void)
{
	print("%s\n", __func__);
	int x;

	if(!swenabled)
		return;
	swmove(mousexy());
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
	print("%s\n", __func__);
	static int init;

	if(!init){
		init = 1;
		addclock0link(swcursorclock, 10);
	}
	if(swback){
		freememimage(swback);
		freememimage(swmask);
		freememimage(swmask1);
		freememimage(swimg);
		freememimage(swimg1);
	}

	swback  = allocmemimage(Rect(0,0,32,32), gscreen->chan);
	swmask  = allocmemimage(Rect(0,0,16,16), GREY8);
	swmask1 = allocmemimage(Rect(0,0,16,16), GREY1);
	swimg   = allocmemimage(Rect(0,0,16,16), GREY8);
	swimg1  = allocmemimage(Rect(0,0,16,16), GREY1);
	if(swback==nil || swmask==nil || swmask1==nil || swimg==nil || swimg1 == nil){
		print("software cursor: allocmemimage fails\n");
		return;
	}

	memfillcolor(swmask, DOpaque);
	memfillcolor(swmask1, DOpaque);
	memfillcolor(swimg, DBlack);
	memfillcolor(swimg1, DBlack);
}

int
screeninit(void)
{
	print("%s\n", __func__);
	static int first = 1;
	struct cb_framebuffer *fb;
	print("%s: fb is %p\n", __func__, cbinfo.framebuffer);
	if (!cbinfo.framebuffer)
		return -1;

	fb = KADDR((uintptr_t)cbinfo.framebuffer);
	framebuf.pixel =  KADDR(fb->physical_address);

	/* should not happen but ...*/
	if (!framebuf.pixel)
		return -1;

	print("%s: pixels are %p\n", __func__, fb->physical_address);

	if (first) {
		iprint("screeninit...");
		lcdstop();
	}

	lcdinit();
	lcdon(1);
	if (first) {
		memimageinit();
		memdefont = getmemdefont();
		screentest();
	}

	xgdata.ref = 1;
	xgdata.bdata = (uint8_t *)framebuf.pixel;

	gscreen = &xgscreen;
	gscreen->r = Rect(0, 0, Wid, Ht);
	gscreen->clipr = gscreen->r;
	/* width, in words, of a single scan line */
	gscreen->width = Wid * (Depth / BI2BY) / BY2WD;
	flushmemscreen(gscreen->r);

	blanktime = 3;				/* minutes */

	if (first) {
		iprint("on: blue for 3 seconds...");
		delay(3*1000);
		iprint("\n");

		screenwin();		/* draw border & top orange bar */
		// ? screenputs = corebootcreenputs;
		iprint("screen: frame buffer at %#p for %dx%d\n",
			framebuf.pixel, Cbscreen.wid, Cbscreen.ht);

		swenabled = 1;
		swcursorinit();		/* needs gscreen set */
		setcursor(&arrow);

		first = 0;
	}
	return 0;
}

/* flushmemscreen should change buffer? */
void
flushmemscreen(Rectangle r)
{
	print("%s\n", __func__);
	uintptr_t start, end;

	if (r.min.x < 0)
		r.min.x = 0;
	if (r.max.x > Wid)
		r.max.x = Wid;
	if (r.min.y < 0)
		r.min.y = 0;
	if (r.max.y > Ht)
		r.max.y = Ht;
	if (rectclip(&r, gscreen->r) == 0)
		return;
	start = (uintptr_t)&framebuf.pixel[r.min.y*Wid + r.min.x];
	end   = (uintptr_t)&framebuf.pixel[(r.max.y - 1)*Wid + r.max.x -1];
	print("Flushmemscreen %p %p\n", start, end);
	// for now. Don't think we need it. cachedwbse((uint32_t *)start, end - start);
	coherence();
}

/*
 * export screen to devdraw
 */
uint8_t*
attachscreen(Rectangle *r, uint32_t *chan, int *d, int *width, int *softscreen)
{
	print("%s\n", __func__);
	if (screeninit() < 0)
		return nil;
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = (landscape == 0);
	return (uint8_t *)gscreen->data->bdata;
}

void
getcolor(uint32_t p, uint32_t *pr, uint32_t *pg, uint32_t *pb)
{
	print("%s\n", __func__);
	//USED(p, pr, pg, pb);
}

int
setcolor(uint32_t p, uint32_t r, uint32_t g, uint32_t b)
{
	print("%s\n", __func__);
	//USED(p, r, g, b);
	return 0;
}

void
blankscreen(int blank)
{
	print("%s\n", __func__);
	if (blank)
		lcdon(0);
	else {
		lcdinit();
		lcdon(1);
	}
}

/* Not sure if we ever need this ... */
#if 0
void
corebootscreenputs(char *s, int n)
{
	print("%s\n", __func__);
	int i;
	Rune r;
	char buf[4];

	if (!islo()) {
		/* don't deadlock trying to print in interrupt */
		if (!canlock(&screenlock))
			return;			/* discard s */
	} else
		lock(&screenlock);

	while (n > 0) {
		i = chartorune(&r, s);
		if (i == 0) {
			s++;
			--n;
			continue;
		}
		memmove(buf, s, i);
		buf[i] = 0;
		n -= i;
		s += i;
		screenputc(buf);
	}
	unlock(&screenlock);
}
#endif
static void
screenwin(void)
{
	print("%s\n", __func__);
	char *greet;
	Memimage *orange;
	Point p; //, q;
	Rectangle r;

	memsetchan(gscreen, RGB16);

	back = memwhite;
	conscol = memblack;

	orange = allocmemimage(Rect(0, 0, 1, 1), RGB16);
	orange->flags |= Frepl;
	orange->clipr = gscreen->r;
	orange->data->bdata[0] = 0x40;		/* magic: colour? */
	orange->data->bdata[1] = 0xfd;		/* magic: colour? */

	w = memdefont->info[' '].width;
	h = memdefont->height;

	r = insetrect(gscreen->r, 4);

	memimagedraw(gscreen, r, memblack, ZP, memopaque, ZP, S);
	window = insetrect(r, 4);
	memimagedraw(gscreen, window, memwhite, ZP, memopaque, ZP, S);

	memimagedraw(gscreen, Rect(window.min.x, window.min.y,
		window.max.x, window.min.y + h + 5 + 6), orange, ZP, nil, ZP, S);
	freememimage(orange);
	window = insetrect(window, 5);

	greet = " Plan 9 Console ";
	p = addpt(window.min, Pt(10, 0));
	//q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	flushmemscreen(r);
	window.min.y += h + 6;
	curpos = window.min;
	window.max.y = window.min.y + ((window.max.y - window.min.y) / h) * h;
}

static int
scroll(VGAscr* _1, Rectangle _2, Rectangle _3)
{
	print("%s\n", __func__);
	int o;
	Point p;
	Rectangle r;

	/* move window contents up Scroll text lines */
	o = Scroll * h;
	r = Rpt(window.min, Pt(window.max.x, window.max.y - o));
	p = Pt(window.min.x, window.min.y + o);
	memimagedraw(gscreen, r, gscreen, p, nil, p, S);
	flushmemscreen(r);

	/* clear the bottom Scroll text lines */
	r = Rpt(Pt(window.min.x, window.max.y - o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP, S);
	flushmemscreen(r);

	curpos.y -= o;
	return 0;
}

#if 0
void
screenputc(char *buf)
{
	print("%s\n", __func__);
	int w;
	uint pos;
	Point p;
	Rectangle r;
	static int *xp;
	static int xbuf[256];

	if (xp < xbuf || xp >= &xbuf[sizeof(xbuf)])
		xp = xbuf;

	switch (buf[0]) {
	case '\n':
		if (curpos.y + h >= window.max.y)
			scroll();
		curpos.y += h;
		screenputc("\r");
		break;
	case '\r':
		xp = xbuf;
		curpos.x = window.min.x;
		break;
	case '\t':
		p = memsubfontwidth(memdefont, " ");
		w = p.x;
		if (curpos.x >= window.max.x - Tabstop * w)
			screenputc("\n");

		pos = (curpos.x - window.min.x) / w;
		pos = Tabstop - pos % Tabstop;
		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x + pos * w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		flushmemscreen(r);
		curpos.x += pos * w;
		break;
	case '\b':
		if (xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		flushmemscreen(r);
		curpos.x = *xp;
		break;
	case '\0':
		break;
	default:
		p = memsubfontwidth(memdefont, buf);
		w = p.x;

		if (curpos.x >= window.max.x - w)
			screenputc("\n");

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x + w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		flushmemscreen(r);
		curpos.x += w;
	}
}
#endif

