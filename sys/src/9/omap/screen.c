/*
 * ti omap35 display subsystem (dss)
 *
 * can handle 2ⁿ bits per pixel for 0 < n ≤ 4, and 12 and 24 bits.
 * can handle	1024×768 at 60 Hz with pixel clock of 63.5 MHz
 *		1280×800 at 59.91 Hz with pixel clock of 71 MHz
 *		1400×1050 lcd at 50 MHz with pixel clock of 75 MHz
 * has 256 24-bit entries in RGB palette
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
#include "screen.h"
// #include "gamma.h"

enum {
	Tabstop	= 4,		/* should be 8 */
	Scroll	= 8,		/* lines to scroll at one time */
	/*
	 * screen settings for Wid and Ht, should a bit more dynamic?
	 * http://www.epanorama.net/faq/vga2rgb/calc.html
	 * used to calculate settings.
	 */

//	Hbp     = (248-1) << 20,
//	Hfp     = (48-1) << 8,
//	Hsw     = 112-1,

//	Vbp     = 38 << 20,
//	Vfp     = 1 << 8,
//	Vsw     = 3,

	Tft	= 0x60,

	Loadmode = 2 << 1,
	Fifosize = 0x400,

	/* dispc sysconfig */
	Midlemode	= 2 << 12,
	Sidlemode	= 2 << 3,
	EnableWakeup	= 1 << 2,
	Autoidle	= 1 << 0,

	/* dispc pool_freq */
	Ipc		= 1 << 14,
	Ihs		= 1 << 13,
	Ivs		= 1 << 12,
	Acb		= 0x28,

	/* gfx attribs */
	Burstsize	= 2 << 6,
	Format		= 6 << 1,
	Gfxenable	= 1 << 0,

	/* dispc control */
	Gpout1		= 1 << 16,
	Gpout0		= 1 << 15,
	Tftdata		= 3 << 8,
	Digital		= 1 << 6,
	Lcd		= 1 << 5,
	Stntft		= 1 << 3,
	Digitalen	= 1 << 1,
//	Lcden		= 1 << 0,	/* unused */
};

typedef struct Dispcregs Dispc;
typedef struct Dssregs Dss;
typedef struct Ioregs Ioregs;

struct Ioregs {				/* common registers, 68 (0x44) bytes */
	ulong 	rev;
	uchar	_pad0[0x10-0x4];
	ulong	sysconf;
	ulong	sysstat;
	ulong	irqstat1;

	/* Dispc only regs */
	ulong	irqen1;
	ulong	wkupen;
	ulong	_pad1;
	ulong	irqsts2;
	ulong	irqen2;
	ulong	_pad2[4];

	ulong	ctrl;
};

struct Dssregs {			/* display subsys at 0x48050000 */
	Ioregs;
	ulong	sdicrtl;
	ulong	pllcrtl;
	uchar	_pad3[0x5c-0x4c];
	ulong	sdistat;
};

struct Dispcregs {			/* display ctlr at 0x48050400 */
	Ioregs;
	ulong	config;
	ulong	_pad3;
	ulong 	defaultcolor[2];
	ulong	transcolor[2];
	ulong	linestat;
	ulong	linenum;
	ulong	timing_h;
	ulong	timing_v;
	ulong	pol_req;
	ulong	divisor;
	ulong	alpha;
	ulong	digsize;
	ulong	lcdsize;

	ulong	base[2];	/* should allocate both to avoid dithering */
	ulong	pos;
	ulong	size;
	ulong	_pad4[4];
	ulong	attrib;
	ulong	fifothr;
	ulong	fifosize;
	ulong	rowinc;
	ulong	pixelinc;
	ulong	winskip;
	ulong	palette;		/* gfx_table_ba */
	uchar	_pad5[0x5d4 - 0x4bc];

	ulong	datacycle[3];
	uchar	_pad5[0x620 - 0x5e0];

	ulong	cprcoefr;
	ulong	cprcoefg;
	ulong	cprcoefb;
	ulong	preload;
};

int	drawdebug;
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

OScreen oscreen;
Settings settings[] = {
[Res800x600]   {  800,  600, 60, RGB16,  40000,	 88, 40, 128,	23, 1, 5, },
[Res1024x768]  { 1024,  768, 60, RGB16,  65000,	160, 24, 136,	29, 3, 7, },
[Res1280x1024] { 1280, 1024, 60, RGB16, 108000,	248, 48, 112,	38, 1, 4, },
[Res1400x1050] { 1400, 1050, 50, RGB16, 108000, 248, 48, 112,	38, 1, 4, }, // TODO
};
Omap3fb *framebuf;
Memimage *gscreen;

static Memdata xgdata;

static Memimage xgscreen =
{
	{ 0, 0, Wid, Ht },	/* r */
	{ 0, 0, Wid, Ht },	/* clipr */
	Depth,			/* depth */
	3,			/* nchan */
	RGB16,			/* chan */
	nil,			/* cmap */
	&xgdata,		/* data */
	0,			/* zero */
	Wid*(Depth/BI2BY)/BY2WD, /* width in words of a single scan line */
	0,			/* layer */
	0,			/* flags */
};

static Memimage *conscol;
static Memimage *back;

static Memsubfont *memdefont;

static Lock screenlock;

static Point	curpos;
static int	h, w;
static int	landscape = 0;	/* screen orientation, default is 0: portrait */
static ushort	*vscreen;	/* virtual screen */
static Rectangle window;

static Dispc *dispc = (Dispc *)PHYSDISPC;
static Dss *dss	= (Dss *)PHYSDSS;

static	void	omapscreenputs(char *s, int n);
static	ulong	rep(ulong, int);
static	void	screenputc(char *buf);
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
	dispc->ctrl &= ~1;		/* disable the lcd */
	coherence();

	dispc->irqstat1 |= 1;		/* set framedone */
	coherence();

	/* the lcd never comes ready, so don't bother with this */
#ifdef notdef
	/* spin until the frame is complete, but not forever */
	for(cnt = 50; !(dispc->irqstat1 & 1) && cnt-- > 0; )
		delay(10);
#endif
	delay(20);			/* worst case for 1 frame, 50Hz */
}

static void
dssstart(void)
{
	/* should reset the dss system */
	dss->sysconf |= 1;
	coherence();
}

/* see spruf98i §15.6.7.4.2 */
static void
configdispc(void)
{
	Settings *sp;

	sp = oscreen.settings;
	dss->ctrl &= 0x78;		/* choose dss clock */
	dispc->sysconf = Midlemode | Sidlemode | EnableWakeup | Autoidle;
	dispc->config = Loadmode;
	coherence();

	/* pll */
	dispc->defaultcolor[0] = 0;	/* set background color to black? */
	dispc->defaultcolor[1] = 0;
	dispc->transcolor[0] = 0;	/* set transparency to full */
	dispc->transcolor[1] = 0;

	dispc->timing_h = (sp->hbp-1) << 20 | (sp->hfp-1) << 8 |
			(sp->hsw-1);
	dispc->timing_v = sp->vbp << 20 | sp->vfp << 8 |
			(sp->vsw-1);

	dispc->pol_req = Ipc | Ihs | Ivs | Acb;
	dispc->divisor = 1 << 16 | HOWMANY(432000, sp->pixelclock);

	dispc->lcdsize = (sp->ht - 1) << 16 | (sp->wid - 1);
	coherence();

	dispc->base[0] = PADDR(framebuf->pixel);
	dispc->base[1] = PADDR(framebuf->pixel);

	dispc->pos = 0;			/* place screen in the left corner */
	/* use the whole screen */
	dispc->size = (sp->ht - 1) << 16 | (sp->wid - 1);

	/* what mode does plan 9 use for fb? */
	dispc->attrib = Burstsize | Format | Gfxenable;

	dispc->preload = Tft;
	dispc->fifosize = Fifosize;
	/* 1008 is max for our Burstsize */
	dispc->fifothr = (Fifosize - 1) << 16 | (1008 - 1);

	/* 1 byte is one pixel (not true, we use 2 bytes per pixel) */
	dispc->rowinc = 1;
	dispc->pixelinc = 1;
	dispc->winskip = 0;		/* don't skip anything */
	coherence();

	// dispc->palette = PADDR(framebuf->palette);
}

static void
lcdon(int enable)
{
	dispc->ctrl = Gpout1 | Gpout0 | Tftdata | Digital | Lcd | Stntft |
		Digitalen | enable;
	coherence();
	delay(10);
}

static void
lcdstop(void)
{
	configscreengpio();
	screenclockson();

	lcdoff();
}

static void
lcdinit(void)
{
	lcdstop();

	dssstart();
	configdispc();
}

/* Paint the image data with blue pixels */
void
screentest(void)
{
	int i;

	for (i = nelem(framebuf->pixel) - 1; i >= 0; i--)
		framebuf->pixel[i] = 0x1f;			/* blue */
//	memset(framebuf->pixel, ~0, sizeof framebuf->pixel);	/* white */
}

void
screenpower(int on)
{
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
	if (dolock)
		lock(&oscreen);
	cursoroff(0);
	swcursordraw();
	if (dolock)
		unlock(&oscreen);
	return 0;
}

void
cursoroff(int dolock)
{
	if (dolock)
		lock(&oscreen);
	swcursorhide();
	if (dolock)
		unlock(&oscreen);
}

void
swload(Cursor *curs)
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
	memimagedraw(swimg1,  swimg1->r,  swimg,  ZP, memopaque, ZP, S);
	memimagedraw(swmask1, swmask1->r, swmask, ZP, memopaque, ZP, S);
}

/* called from devmouse */
void
setcursor(Cursor* curs)
{
	cursoroff(1);
	oscreen.Cursor = *curs;
	swload(curs);
	cursoron(1);
}

int
swmove(Point p)
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

/* called from main and possibly later from devdss to change resolution */
void
screeninit(void)
{
	static int first = 1;

	if (first) {
		iprint("screeninit...");
		oscreen.settings = &settings[Res1280x1024];

		lcdstop();
		if (framebuf)
			free(framebuf);
		/* mode is 16*32 = 512 */
		framebuf = xspanalloc(sizeof *framebuf, 16*32, 0);
	}

	lcdinit();
	lcdon(1);
	if (first) {
		memimageinit();
		memdefont = getmemdefont();
		screentest();
	}

	xgdata.ref = 1;
	xgdata.bdata = (uchar *)framebuf->pixel;

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
		screenputs = omapscreenputs;
		iprint("screen: frame buffer at %#p for %dx%d\n",
			framebuf, oscreen.settings->wid, oscreen.settings->ht);

		swenabled = 1;
		swcursorinit();		/* needs gscreen set */
		setcursor(&arrow);

		first = 0;
	}
}

/* flushmemscreen should change buffer? */
void
flushmemscreen(Rectangle r)
{
	ulong start, end;

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
	start = (ulong)&framebuf->pixel[r.min.y*Wid + r.min.x];
	end   = (ulong)&framebuf->pixel[(r.max.y - 1)*Wid + r.max.x -1];
	cachedwbse((ulong *)start, end - start);
}

/*
 * export screen to devdraw
 */
uchar*
attachscreen(Rectangle *r, ulong *chan, int *d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = (landscape == 0);
	return (uchar *)gscreen->data->bdata;
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	USED(p, pr, pg, pb);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p, r, g, b);
	return 0;
}

void
blankscreen(int blank)
{
	if (blank)
		lcdon(0);
	else {
		lcdinit();
		lcdon(1);
	}
}

static void
omapscreenputs(char *s, int n)
{
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

static void
screenwin(void)
{
	char *greet;
	Memimage *orange;
	Point p, q;
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
	q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	flushmemscreen(r);
	window.min.y += h + 6;
	curpos = window.min;
	window.max.y = window.min.y + ((window.max.y - window.min.y) / h) * h;
}

static void
scroll(void)
{
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
}

static void
screenputc(char *buf)
{
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
