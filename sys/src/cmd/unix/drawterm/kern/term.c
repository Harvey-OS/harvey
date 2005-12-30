#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#include	<draw.h>
#include	<memdraw.h>
#include	"screen.h"

#define	MINX	8
#define	Backgnd		0xFF	/* white */

		Memsubfont	*memdefont;
		
struct{
	Point	pos;
	int	bwid;
}out;

Lock	screenlock;

Memimage *conscol;
Memimage *back;
extern Memimage *gscreen;

static Rectangle flushr;
static Rectangle window;
static Point curpos;
static int h;
static void	termscreenputs(char*, int);


static void
screenflush(void)
{
	drawflushr(flushr);
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
screenwin(void)
{
	Point p;
	char *greet;
	Memimage *grey;

	drawqlock();
	back = memwhite;
	conscol = memblack;
	memfillcolor(gscreen, 0x444488FF);
	
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
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	window.min.y += h+6;
	curpos = window.min;
	window.max.y = window.min.y+((window.max.y-window.min.y)/h)*h;
	flushmemscreen(gscreen->r);
	drawqunlock();
}

void
terminit(void)
{
	memdefont = getmemdefont();
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = memdefont->info[' '].width;
	screenwin();
	screenputs = termscreenputs;
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

	switch(buf[0]) {
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
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		addflush(r);
		curpos.x += pos*w;
		break;
	case '\b':
		if(xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
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
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		addflush(r);
		curpos.x += w;
	}
}

static void
termscreenputs(char *s, int n)
{
	int i, locked;
	Rune r;
	char buf[4];

	lock(&screenlock);
	locked = drawcanqlock();
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
	if(locked)
		drawqunlock();
	screenflush();
	unlock(&screenlock);
}
