#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"kbd.h"
#include	"error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"screen.h"

enum
{
	CURSORDIM = 16
};

Memimage	*gscreen;
Point		ZP;
int		cursorver;
Point		cursorpos;

static Memimage		*back;
static Memimage		*conscol;
static Memimage		*curscol;
static Point		curpos;
static Memsubfont	*memdefont;
static Rectangle	flushr;
static Rectangle	window;
static int		h;
static int		w;

static Rectangle	cursorr;
static Point		offscreen;
static uchar		cursset[CURSORDIM*CURSORDIM/8];
static uchar		cursclr[CURSORDIM*CURSORDIM/8];
static int		cursdrawvers = -1;
static Memimage		*cursorset;
static Memimage		*cursorclear;
static Cursor		screencursor;

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

void
screeninit(int x, int y, char *chanstr)
{
	Point p, q;
	char *greet;
	char buf[128];
	Memimage *grey;
	Rectangle r;
	int chan;

	cursorver = 0;

	memimageinit();
	chan = strtochan(chanstr);
	if(chan == 0)
		error("bad screen channel string");

	r = Rect(0, 0, x, y);
	gscreen = allocmemimage(r, chan);
	if(gscreen == nil){
		snprint(buf, sizeof buf, "can't allocate screen image: %r");
		error(buf);
	}

	offscreen = Pt(x + 100, y + 100);
	cursorr = Rect(0, 0, CURSORDIM, CURSORDIM);
	cursorset = allocmemimage(cursorr, GREY8);
	cursorclear = allocmemimage(cursorr, GREY1);
	if(cursorset == nil || cursorclear == nil){
		freememimage(gscreen);
		freememimage(cursorset);
		freememimage(cursorclear);
		gscreen = nil;
		cursorset = nil;
		cursorclear = nil;
		snprint(buf, sizeof buf, "can't allocate cursor images: %r");
		error(buf);
	}

	drawlock();

	/*
	 * set up goo for screenputs
	 */
	memdefont = getmemdefont();

	back = memwhite;
	conscol = memblack;

	/* a lot of work to get a grey color */
	curscol = allocmemimage(Rect(0,0,1,1), RGBA32);
	curscol->flags |= Frepl;
	curscol->clipr = gscreen->r;
	memfillcolor(curscol, 0xff0000ff);

	memfillcolor(gscreen, 0x444488FF);

	w = memdefont->info[' '].width;
	h = memdefont->height;

	window.min = addpt(gscreen->r.min, Pt(20,20));
	window.max.x = window.min.x + Dx(gscreen->r)*3/4-40;
	window.max.y = window.min.y + Dy(gscreen->r)*3/4-100;

	memimagedraw(gscreen, window, memblack, ZP, memopaque, ZP, S);
	window = insetrect(window, 4);
	memimagedraw(gscreen, window, memwhite, ZP, memopaque, ZP, S);

	/* a lot of work to get a grey color */
	grey = allocmemimage(Rect(0,0,1,1), CMAP8);
	grey->flags |= Frepl;
	grey->clipr = gscreen->r;
	memfillcolor(grey, 0xAAAAAAFF);
	memimagedraw(gscreen, Rect(window.min.x, window.min.y,
			window.max.x, window.min.y+h+5+6), grey, ZP, nil, ZP, S);
	freememimage(grey);
	window = insetrect(window, 5);

	greet = " Plan 9 Console ";
	p = addpt(window.min, Pt(10, 0));
	q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	window.min.y += h+6;
	curpos = window.min;
	window.max.y = window.min.y+((window.max.y-window.min.y)/h)*h;
	flushmemscreen(gscreen->r);

	drawunlock();

	setcursor(&arrow);
}

uchar*
attachscreen(Rectangle* r, ulong* chan, int* d, int* width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 1;

	return gscreen->data->bdata;
}

void
getcolor(ulong , ulong* pr, ulong* pg, ulong* pb)
{
	*pr = 0;
	*pg = 0;
	*pb = 0;
}

int
setcolor(ulong , ulong , ulong , ulong )
{
	return 0;
}

/*
 * called with cursor unlocked, drawlock locked
 */
void
cursordraw(Memimage *dst, Rectangle r)
{
	static uchar set[CURSORDIM*CURSORDIM], clr[CURSORDIM*CURSORDIM/8];
	static int ver = -1;
	int i, j, n;

	lock(&cursor);
	if(ver != cursorver){
		n = 0;
		for(i = 0; i < CURSORDIM*CURSORDIM/8; i += CURSORDIM/8){
			for(j = 0; j < CURSORDIM; j++){
				if(cursset[i + (j >> 3)] & (1 << (7 - (j & 7))))
					set[n] = 0xaa;
				else
					set[n] = 0;
				n++;
			}
		}
		memmove(clr, cursclr, CURSORDIM*CURSORDIM/8);
		ver = cursorver;
		unlock(&cursor);
		loadmemimage(cursorset, cursorr, set, CURSORDIM*CURSORDIM);
		loadmemimage(cursorclear, cursorr, clr, CURSORDIM*CURSORDIM/8);
	}else
		unlock(&cursor);
	memimagedraw(dst, r, memwhite, ZP, cursorclear, ZP, SoverD);
	memimagedraw(dst, r, curscol, ZP, cursorset, ZP, SoverD);
}

/*
 * called with cursor locked, drawlock possibly unlocked
 */
Rectangle
cursorrect(void)
{
	Rectangle r;

	r.min.x = cursorpos.x + cursor.offset.x;
	r.min.y = cursorpos.y + cursor.offset.y;
	r.max.x = r.min.x + CURSORDIM;
	r.max.y = r.min.y + CURSORDIM;
	return r;
}

/*
 * called with cursor locked, drawlock possibly unlocked
 */
void
setcursor(Cursor* curs)
{
	cursorver++;
	memmove(cursset, curs->set, CURSORDIM*CURSORDIM/8);
	memmove(cursclr, curs->clr, CURSORDIM*CURSORDIM/8);
}

int
cursoron(int dolock)
{
	if(dolock)
		lock(&cursor);
	cursorpos = mousexy();
	if(dolock)
		unlock(&cursor);

	return 0;
}

void
cursoroff(int dolock)
{
	if(dolock)
		lock(&cursor);
	cursorpos = offscreen;
	if(dolock)
		unlock(&cursor);
}

void
blankscreen(int blank)
{
	USED(blank);
}

static void
screenflush(void)
{
	flushmemscreen(flushr);
	flushr = Rect(10000, 10000, -10000, -10000);
}

static void
addflush(Rectangle r)
{
	if(flushr.min.x >= flushr.max.x)
		flushr = r;
	else
		combinerect(&flushr, r);
}

static void
scroll(void)
{
	int o;
	Point p;
	Rectangle r;

	o = 8*h;
	r = Rpt(window.min, Pt(window.max.x, window.max.y-o));
	p = Pt(window.min.x, window.min.y+o);
	memimagedraw(gscreen, r, gscreen, p, nil, p, S);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP, S);
	flushmemscreen(gscreen->r);

	curpos.y -= o;
}

static void
screenputc(char *buf)
{
	Point p;
	int w, pos;
	Rectangle r;
	static int *xp;
	static int xbuf[256];

	if(xp < xbuf || xp >= &xbuf[sizeof(xbuf)])
		xp = xbuf;

	switch(buf[0]){
	case '\n':
		if(curpos.y+h >= window.max.y)
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
		*xp++ = curpos.x;
		pos = (curpos.x-window.min.x)/w;
		pos = 8-(pos%8);
		r = Rect(curpos.x, curpos.y, curpos.x+pos*w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, memopaque, ZP, S);
		addflush(r);
		curpos.x += pos*w;
		break;
	case '\b':
		if(xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, memopaque, ZP, S);
		addflush(r);
		curpos.x = *xp;
		break;
	default:
		p = memsubfontwidth(memdefont, buf);
		w = p.x;

		if(curpos.x >= window.max.x-w)
			screenputc("\n");

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, memopaque, ZP, S);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		addflush(r);
		curpos.x += w;
	}
}

void
screenputs(char *s, int n)
{
	int i;
	Rune r;
	char buf[4];

	drawlock();
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
	screenflush();
	drawunlock();
}
