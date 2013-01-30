/*
 * bcm2385 framebuffer
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

enum {
	Tabstop		= 4,
	Scroll		= 8,
	Wid		= 1024,
	Ht		= 768,
	Depth		= 16,
};

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
	0, 			/* width in words of a single scan line */
	0,			/* layer */
	0,			/* flags */
};

static Memimage *conscol;
static Memimage *back;
static Memsubfont *memdefont;

static Lock screenlock;

static Point	curpos;
static int	h, w;
static Rectangle window;

static void myscreenputs(char *s, int n);
static void screenputc(char *buf);
static void screenwin(void);

/*
 * Software cursor. 
 */
static int	swvisible;	/* is the cursor visible? */
static int	swenabled;	/* is the cursor supposed to be on the screen? */
static Memimage *swback;	/* screen under cursor */
static Memimage *swimg;		/* cursor image */
static Memimage *swmask;	/* cursor mask */
static Memimage *swimg1;
static Memimage *swmask1;

static Point	swoffset;
static Rectangle swrect;	/* screen rectangle in swback */
static Point	swpt;		/* desired cursor location */
static Point	swvispt;	/* actual cursor location */
static int	swvers;		/* incremented each time cursor image changes */
static int	swvisvers;	/* the version on the screen */

/*
 * called with drawlock locked for us, most of the time.
 * kernel prints at inopportune times might mean we don't
 * hold the lock, but memimagedraw is now reentrant so
 * that should be okay: worst case we get cursor droppings.
 */
static void
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

static void
swcursoravoid(Rectangle r)
{
	if(swvisible && rectXrect(r, swrect))
		swcursorhide();
}

static void
swcursordraw(void)
{
	int dounlock;

	if(swvisible)
		return;
	if(swenabled == 0)
		return;
	if(swback == nil || swimg1 == nil || swmask1 == nil)
		return;
	dounlock = canqlock(&drawlock);
	swvispt = swpt;
	swvisvers = swvers;
	swrect = rectaddpt(Rect(0,0,16,16), swvispt);
	memimagedraw(swback, swback->r, gscreen, swpt, memopaque, ZP, S);
	memimagedraw(gscreen, swrect, swimg1, ZP, swmask1, ZP, SoverD);
	flushmemscreen(swrect);
	swvisible = 1;
	if(dounlock)
		qunlock(&drawlock);
}

int
cursoron(int dolock)
{
	int retry;

	if (dolock)
		lock(&cursor);
	if (canqlock(&drawlock)) {
		retry = 0;
		swcursorhide();
		swcursordraw();
		qunlock(&drawlock);
	} else
		retry = 1;
	if (dolock)
		unlock(&cursor);
	return retry;
}

void
cursoroff(int dolock)
{
	if (dolock)
		lock(&cursor);
	swcursorhide();
	if (dolock)
		unlock(&cursor);
}

static void
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
	cursoroff(0);
	swload(curs);
	cursoron(0);
}

static int
swmove(Point p)
{
	swpt = addpt(p, swoffset);
	return 0;
}

static void
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
		swenabled = 1;
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
hwdraw(Memdrawparam *par)
{
	Memimage *dst, *src, *mask;

	if((dst=par->dst) == nil || dst->data == nil)
		return 0;
	if((src=par->src) == nil || src->data == nil)
		return 0;
	if((mask=par->mask) == nil || mask->data == nil)
		return 0;

	if(dst->data->bdata == xgdata.bdata)
		swcursoravoid(par->r);
	if(src->data->bdata == xgdata.bdata)
		swcursoravoid(par->sr);
	if(mask->data->bdata == xgdata.bdata)
		swcursoravoid(par->mr);

	return 0;
}

static int
screensize(void)
{
	char *p;
	char *f[3];
	int width, height, depth;

	p = getconf("vgasize");
	if(p == nil || getfields(p, f, nelem(f), 0, "x") != nelem(f) ||
	    (width = atoi(f[0])) < 16 ||
	    (height = atoi(f[1])) <= 0 ||
	    (depth = atoi(f[2])) <= 0)
		return -1;
	xgscreen.r.max = Pt(width, height);
	xgscreen.depth = depth;
	return 0;
}

void
screeninit(void)
{
	uchar *fb;
	int set;
	ulong chan;

	set = screensize() == 0;
	fb = fbinit(set, &xgscreen.r.max.x, &xgscreen.r.max.y, &xgscreen.depth);
	if(fb == nil){
		print("can't initialise %dx%dx%d framebuffer \n",
			xgscreen.r.max.x, xgscreen.r.max.y, xgscreen.depth);
		return;
	}
	xgscreen.clipr = xgscreen.r;
	switch(xgscreen.depth){
	default:
		print("unsupported screen depth %d\n", xgscreen.depth);
		xgscreen.depth = 16;
		/* fall through */
	case 16:
		chan = RGB16;
		break;
	case 24:
		chan = BGR24;
		break;
	case 32:
		chan = ARGB32;
		break;
	}
	memsetchan(&xgscreen, chan);
	conf.monitor = 1;
	xgdata.bdata = fb;
	xgdata.ref = 1;
	gscreen = &xgscreen;
	gscreen->width = wordsperline(gscreen->r, gscreen->depth);

	memimageinit();
	memdefont = getmemdefont();
	screenwin();
	screenputs = myscreenputs;
}

void
flushmemscreen(Rectangle)
{
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int* d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 0;

	return gscreen->data->bdata;
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
	fbblank(blank);
}

static void
myscreenputs(char *s, int n)
{
	int i;
	Rune r;
	char buf[4];

	if(!islo()) {
		/* don't deadlock trying to print in interrupt */
		if(!canlock(&screenlock))
			return;	
	}
	else
		lock(&screenlock);

	while(n > 0){
		i = chartorune(&r, s);
		if(i == 0){
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

	o = Scroll*h;
	r = Rpt(window.min, Pt(window.max.x, window.max.y-o));
	p = Pt(window.min.x, window.min.y+o);
	memimagedraw(gscreen, r, gscreen, p, nil, p, S);
	flushmemscreen(r);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
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
		break;
	}
}
