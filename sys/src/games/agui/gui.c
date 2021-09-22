#include	<audio.h>

/* todo:
 *	scroll bar
 *	get text to do something when selected
 *	get some sort of bsearch on text
 */
#define	Basic	10

#define	Butdx	(Basic*7.0)
#define	Butdy	(Basic*3.0)
#define	Lindx	(Basic*10.0)
#define	Lindy	(Basic*2.0)
//#define	Linbdy	(Basic*1.6)
#define	Linbdy	(font->height)
#define	Border	(Basic*0.5)
#define	Bar	(Basic*1.5)

enum
{

	Color1	= DPaleyellow,		/* yellow */
	Color2	= DDarkyellow,
	Color3	= DPalebluegreen,	/* blue green */
	Color4	= DPalegreygreen,
	Color5	= DYellowgreen,		/* green */
	Color6	= DPurpleblue,
	Color7	= 0x9eee4fFF,		/* lime puke */
	Color	= Color7,

	Butup	= 0,
	Butdn,
	Butcont,
	Butmiss,
	Butinit
};

typedef	struct	Frame	Frame;
typedef	void	Fn(Frame*, int, int, Point*);

struct	Frame
{
	int	type;
	int	param;
	char*	label;
	Fn	*hit;
	Image	fgnd;
	Image*	bgnd;
	int	sel;
};

static	Fn	hitabove;
static	Fn	hitbelow;
static	Fn	hitdown;
static	Fn	hitexit;
static	Fn	hitnow;
static	Fn	hitplaying;
static	Fn	hitqueue;
static	Fn	hitrand;
static	Fn	hitregexp;
static	Fn	hitroot;
static	Fn	hitscroll;
static	Fn	hitskip;
static	Fn	hitup;
static	Fn	hitvol;

static	void	drawlab(Frame*);
static	void	drawllab(Frame*);
static	void	drawshade(Frame*);
static	void	drawabove(Frame*);
static	void	drawbelow(Frame*);
static	void	bhighlight(Frame*, int, Image*);

extern	void	setvolume(int);
extern	int	getvolume(void);

static	Frame*	aboveframe;
static	Frame*	belowframe;
static	Image*	color1;		/* 25% of pure color */
static	Image*	color2;		/* 50% of pure color */
static	Image*	color3;		/* 75% of pure color */
static	Image*	color4;		/* pure color */
static	Frame*	cursongframe;
static	Frame*	lastdown;
static	int	nline;
static	int	obuttons;
static	Frame*	reframe;
static	int	ren;
static	char	restring[100];
static	char	volstring[10];
static	Point	charsize;
static	int	volume;
static	int	firstline;

static
Frame	frame[] =
{
	{ Tlab,0, "<<",		hitup },
	{ Tlab,1, ">>",		hitdown },
	{ Tlab,2, volstring,	hitvol },
	{ Tlab,3, "now",	hitnow },
	{ Tlab,4, "rand",	hitrand },
	{ Tlab,5, "skip",	hitskip },
	{ Tlab,6, "stop",	hitskip },
	{ Tlab,7, "exit",	hitexit },

	{ Tlco,0, "queue>",	hitqueue },
	{ Tlco,1, "root>",	hitroot },

	{ Tmco,0, "playing:",	hitplaying },
	{ Tmco,1, "search:",	hitregexp },

	{ Trco,0, nowplaying,	hitplaying },
	{ Trco,1, restring,	hitregexp },

	{ Tabv,0, "",		hitabove },
	{ Tscr,0, "",		hitscroll },
	{ Tblo,0, "",		hitbelow },
};

void
guiinit(void)
{
	if(initdraw(0, 0, "audio") < 0) {
		print("initdraw %r\n");
		exits("initdraw");
	}
}

void
eresized(int new)
{
	int w, x, y, n, m;
	Frame *p;
	Box r;

	if(new && getwindow(display, Refmesg) < 0) {
		print("can't reattach to window\n");
		exits("reshap");
	}

	for(p=frame; p<frame+nelem(frame); p++) {
		p->fgnd = *screen;
		switch(p->type) {
		default:
			print("unknown frame type %d\n", p->type);
			exits("init");

		case Tlab:		/* top row labels */
			w = p->fgnd.r.max.x - p->fgnd.r.min.x - 2*Border;
			n = p->param;
			m = 8;
			if(volume < 0) {
				m = 7;
				if(n >= 2)
					n--;
			}
			x = p->fgnd.r.min.x + Border +
				(w - Butdx*m) / 2 +n*Butdx;

			y = p->fgnd.r.min.y + Border;
			if(x < 0) {
				print("window not wide enough\n");
				exits("width");
			}
			r = Rect(x,y, x+Butdx,y+Butdy);
			break;
		case Tlco:		/* left column */
			x = p->fgnd.r.min.x + Border;
			y = p->fgnd.r.min.y + Border +
				Butdy + p->param*Lindy;
			r = Rect(x,y, x+Lindx,y+Lindy);
			break;
		case Tmco:		/* middle column */
			x = p->fgnd.r.min.x + Border + Lindx;
			y = p->fgnd.r.min.y + Border + Butdy + p->param*Lindy;
			r = Rect(x,y, x+Lindx,y+Lindy);
			break;
		case Trco:		/* right column */
			x = p->fgnd.r.min.x + Border + Lindx*2;
			y = p->fgnd.r.min.y + Border + Butdy + p->param*Lindy;
			r = Rect(x,y, p->fgnd.r.max.x-Border,y+Lindy);
			break;
		case Tabv:		/* above */
			x = p->fgnd.r.min.x + Border;
			y = p->fgnd.r.min.y + Border + Butdy + 2*Lindy;
			r = Rect(x,y, p->fgnd.r.max.x-Border,y+Lindy);
			break;
		case Tscr:		/* scroll bar */
			x = p->fgnd.r.min.x + Border;
			y = p->fgnd.r.min.y + Border + Butdy + 3*Lindy;
			nline = (p->fgnd.r.max.y - y - Border - 4) / Linbdy;
			r = Rect(x,y, x+Bar,y+nline*Linbdy+4);
			break;
		case Tblo:		/* below */
			x = p->fgnd.r.min.x + Border + Bar;
			y = p->fgnd.r.min.y + Border + Butdy + 3*Lindy;
			if(nline < 10) {
				print("window not tall enough\n");
				exits("height");
			}
			r = Rect(x,y, p->fgnd.r.max.x-Border,y+nline*Linbdy+4);
			break;
		}
		p->fgnd.clipr = r;
	}

	if(color1 == 0) {
		color1 = allocimagemix(display, Color, DWhite);
		color3 = allocimagemix(display, DWhite, Color);
		color4 = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Color);
		if(screen->depth <= 8) {
			/* use stipple pattern */
			color2 = allocimage(display, Rect(0,0,2,2), screen->chan, 1, DWhite);
			draw(color2, Rect(0,0,1,1), color4, nil, ZP);
			draw(color2, Rect(1,1,2,2), color4, nil, ZP);
		} else {
			/* use solid color, blended using alpha */
			Image *mask;
			mask = allocimage(display, Rect(0,0,1,1), GREY8, 1, 0x7F7F7FFF);
			color2 = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DWhite);
			draw(color2, color2->r, color4, mask, ZP);
		}
	}

	if(lastdown && obuttons)
		lastdown->hit(lastdown, obuttons, Butmiss, 0);
	lastdown = 0;
	obuttons = 0;
	firstline = 0;
	draw(screen, screen->r, display->white, nil, ZP);
	for(p=frame; p<frame+nelem(frame); p++)
		p->hit(p, 0, Butinit, 0);

	if(reframe == 0 ||
	   cursongframe == 0 ||
	   aboveframe == 0 ||
	   belowframe == 0) {
		print("no frame\n");
		exits("re");
	}
}

void
mkgui(void)
{
	volume = getvolume();
	charsize = stringsize(font, "t");
	eresized(0);
}

int
inside(Box r, Point p)
{

	if(p.x < r.min.x ||
	   p.x >= r.max.x ||
	   p.y < r.min.y ||
	   p.y >= r.max.y)
		return 0;
	return 1;
}

void
guimouse(Mouse m)
{
	int b, x;
	Frame *p;

	b = m.buttons & (But1|But2|But3);
	if(obuttons == 0 && b == 0)
		return;

	for(p=frame; p < frame+nelem(frame); p++)
		if(inside(p->fgnd.clipr, m.xy))
			break;
	if(p >= frame+nelem(frame))
		p = 0;

/*
 * state machine for each frame for each button.
 *	Butinit (Butdn Butcont* (Butup|Butmiss))*
 */

	if(lastdown != p) {
		/* here mouse not in same window */
		if(lastdown && obuttons)
			lastdown->hit(lastdown, obuttons, Butmiss, 0);
		lastdown = 0;
		if(p && b) {
			p->hit(p, b, Butdn, &m.xy);
			lastdown = p;
		}
	} else
	if(p) {
		/* here mouse in same window */
		if(x = b & obuttons)	/* button continuing */
			p->hit(p, x, Butcont, &m.xy);
		if(x = obuttons & ~b)	/* button release */
			p->hit(p, x, Butup, &m.xy);
		if(x = b & ~obuttons)	/* button down */
			p->hit(p, x, Butdn, &m.xy);
	}
	obuttons = b;
}

void
guikbd(int c)
{
	switch(c) {
	case 'U'-'@':
		ren = 0;
		break;

	case '\b':
		if(ren > 0)
			ren--;
		break;

	case '\n':
		resetline = 1;
		execute(restring);
		return;

	default:
		restring[ren] = c;
		if(ren < nelem(restring)-1)
			ren++;
	}
	restring[ren] = 0;
	drawllab(reframe);
}

void
guiupdate(ulong u)
{

	if(u & Dreshape)
		eresized(0);
	if(u & Dvolume)
		if(volume >= 0)
			setvolume(volume);
	if(u & Dcursong)
		drawllab(cursongframe);
	if(u & Dselect) {
		firstline = 0;
		drawabove(aboveframe);
		drawbelow(belowframe);
	}
	if(u & Dscroll)
		drawbelow(belowframe);
}

static	void
hitup(Frame *f, int, int a, Point*)
{
	int n;

	switch(a) {
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		abovdisp++;
		if(abovdisp >= nabove)
			abovdisp--;
		n = nabove - abovdisp - 1;
		if(n >= 0 && n < nabove)
			expand(above[n], 0);
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	if(a != Butcont)
		drawlab(f);
}

static	void
hitdown(Frame *f, int, int a, Point*)
{
	int n;

	switch(a) {
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		abovdisp--;
		if(abovdisp < 0)
			abovdisp++;
		n = nabove - abovdisp - 1;
		if(n >= 0 && n < nabove)
			expand(above[n], 0);
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	if(a != Butcont)
		drawlab(f);
}

static	void
hitvol(Frame *f, int, int a, Point *p)
{
	int n;

	if(volume < 0)
		return;
	n = volume;
	if(p) {
		n = f->fgnd.clipr.min.x;
		n = (p->x - n) * 100 / (f->fgnd.clipr.max.x - n);
	}
	if(n < 0)
		n = 0;
	if(n > 99)
		n = 99;

	switch(a) {
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		volume = n;
		update |= Dvolume;
		f->bgnd = color1;
		break;
	case Butinit:
	case Butmiss:
		f->bgnd = color1;
		n = volume;
		break;
	}
	strcpy(volstring, "vol  X");
	if(n >= 10)
		volstring[4] = n/10 + '0';
	volstring[5] = n%10 + '0';
	drawlab(f);
}

static	void
hitnow(Frame *f, int, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		now = !now;
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		if(now)
			f->bgnd = color3;
		break;
	}
	drawlab(f);
}

static	void
hitrand(Frame *f, int, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		random = !random;
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		if(random)
			f->bgnd = color3;
		break;
	}
	drawlab(f);
}

static	void
hitskip(Frame *f, int, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		skipsong(strcmp(f->label, "skip"));
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	drawlab(f);
}

static	void
hitexit(Frame *f, int, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		doexit();
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	drawlab(f);
}

static	void
hitqueue(Frame *f, int b, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		if(b & But1)
			expand(Oqueue, 1);
		if(b & But2)
			play(Oqueue);
		if(b & But3)
			mark(Oqueue);
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	drawllab(f);
}

static	void
hitplaying(Frame *f, int b, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		if(b & But1)
			expand(Oplaying, 1);
		if(b & But2)
			play(Oplaying);
		if(b & But2)
			mark(Oplaying);
	case Butmiss:
		f->bgnd = color1;
		break;
	case Butinit:
		cursongframe = f;
		f->bgnd = color1;
		break;
	}
	drawllab(f);
}

static	void
hitroot(Frame *f, int b, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		if(b & But1)
			expand(Oroot, 1);
		if(b & But2)
			play(Oroot);
		if(b & But3)
			mark(Oroot);
	case Butmiss:
	case Butinit:
		f->bgnd = color1;
		break;
	}
	drawllab(f);
}

static	void
hitregexp(Frame *f, int, int a, Point*)
{
	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
	case Butmiss:
		f->bgnd = color1;
		break;
	case Butinit:
		reframe = f;
		f->bgnd = color1;
		break;
	}
	drawllab(f);
}

static	void
hitabove(Frame *f, int b, int a, Point*)
{
	int n;
	long o;

	switch(a) {
	case Butcont:
		return;
	case Butdn:
		f->bgnd = color2;
		break;
	case Butup:
		n = nabove - abovdisp - 1;
		if(n >= 0 && n < nabove) {
			o = above[n];
			if(b & But1)
				expand(o, 1);
			if(b & But2)
				play(o);
			if(b & But3)
				mark(o);
		}
	case Butmiss:
		f->bgnd = color1;
		break;
	case Butinit:
		aboveframe = f;
		f->bgnd = color1;
		break;
	}
	drawabove(f);
}

static	void
hitscroll(Frame *f, int b, int a, Point *p)
{
	int n, fl;

	n = nbelow;
	if(p)
		n = (p->y - f->fgnd.clipr.min.y) / Linbdy;
	if(n < 0)
		n = nbelow;

	switch(a) {
	case Butup:
		fl = firstline;
		if(b & But1)
			fl = firstline - n;
		if(b & But2)
			fl = n;
		if(b & But3)
			fl = firstline + n;
		if(fl > nbelow-1)
			fl = nbelow-1;
		if(fl < 0)
			fl = 0;
		if(fl != firstline) {
			firstline = fl;
			update |= Dscroll;
		}
		break;
	}
}

static	void
hitbelow(Frame *f, int b, int a, Point *p)
{
	int n;
	long o;

	n = nbelow;
	if(p)
		n = (p->y - f->fgnd.clipr.min.y) / Linbdy;
	if(n < 0)
		n = nbelow;
	n += firstline;

	switch(a) {
	case Butdn:
		f->sel = n;
		bhighlight(f, n, color2);
		break;
	case Butup:
		if(n < nbelow) {
			o = below[n];
			if(b & But1)
				expand(o, 1);
			if(b & But2)
				play(o);
			if(b & But3)
				mark(o);
		}
	case Butmiss:
		bhighlight(f, f->sel, color1);
		break;
	case Butcont:
		if(n != f->sel) {
			bhighlight(f, f->sel, color1);
			f->sel = n;
			bhighlight(f, n, color2);
		}
		break;
	case Butinit:
		f->bgnd = color1;
		belowframe = f;
		drawbelow(f);
		break;
	}
}

static	void
drawshade(Frame *f)
{
	Box r;

	r = f->fgnd.clipr;
	draw(&f->fgnd, r, f->bgnd, nil, r.min);
	draw(&f->fgnd,
		Rect(r.min.x,r.min.y, r.min.x+1,r.max.y),
		display->white, nil, ZP);
	draw(&f->fgnd,
		Rect(r.min.x,r.min.y, r.max.x,r.min.y+1),
		display->white, nil, ZP);
	draw(&f->fgnd,
		Rect(r.max.x-1,r.min.y, r.max.x,r.max.y),
		display->black, nil, ZP);
	draw(&f->fgnd,
		Rect(r.min.x,r.max.y-1, r.max.x,r.max.y),
		display->black, nil, ZP);
}

static	void
drawlab(Frame *f)
{
	Point pt;

	drawshade(f);
	pt = stringsize(font, f->label);
	pt.x = f->fgnd.clipr.min.x +
		(f->fgnd.clipr.max.x -
		f->fgnd.clipr.min.x - pt.x) / 2;
	pt.y = f->fgnd.clipr.min.y +
		(f->fgnd.clipr.max.y -
		f->fgnd.clipr.min.y - pt.y) / 2;
	string(&f->fgnd, pt, display->black, ZP, font, f->label);
}

static	void
drawllab(Frame *f)
{
	Point pt;

	drawshade(f);
	pt.x = f->fgnd.clipr.min.x + charsize.x;
	pt.y = f->fgnd.clipr.min.y + (f->fgnd.clipr.max.y - f->fgnd.clipr.min.y - charsize.y) / 2;
	string(&f->fgnd, pt, display->black, ZP, font, f->label);
}

static	void
drawabove(Frame *f)
{
	int i;

	drawshade(f);
	i = nabove - abovdisp - 1;
	if(i >= 0 && i < nabove) {
		aboveframe->label = tstring(above[i]);
		drawllab(f);
	}
}

static	void
drawbelow(Frame *f)
{
	int i;

	drawshade(f);
	for(i=0; i<nline; i++)
		bhighlight(f, i+firstline, 0);
}

static	void
bhighlight(Frame *f, int n, Image *color)
{
	Box r, t;

	if(n < 0 || n >= nbelow ||
	   n < firstline || n >= nline+firstline)
		return;

	r.min.x = f->fgnd.clipr.min.x;
	r.min.y = f->fgnd.clipr.min.y +
		(Linbdy - charsize.y) / 2 +
		(n-firstline)*Linbdy;
	r.max.x = f->fgnd.clipr.max.x - 2;
	r.max.y = r.min.y + Linbdy;
	if(color) {
		t = r;
		t.min.x += 1;
		if(n == firstline)
			t.min.y += 1;
		if(n == nline+firstline-1)
			t.max.y -= 1;
		draw(&f->fgnd, t, color, nil, t.min);
	}
	r.min.x += charsize.x;
	string(&f->fgnd, r.min, display->black, ZP, font, tstring(below[n]));
}
