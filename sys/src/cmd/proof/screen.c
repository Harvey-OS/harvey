#include <u.h>
#include <libg.h>
#include <libc.h>
#include <stdarg.h>
#include <bio.h>
#include "proof.h"

Bitmap	screen;
Mouse 	mouse;

static	int	checkmouse(void);
static	int	buttondown(void);
static	char	*getmousestr(void);
static	char	*getkbdstr(int);

extern	Cursor	blot;
extern	char	*track;

void
mapscreen(void)
{
	binit(0, 0, "proof");
	einit(Ekeyboard|Emouse);
	screen.r = inset(screen.r,3);
}

void
clearscreen(void)
{
	bitblt(&screen, screen.r.min, &screen, screen.r, 0);
}

void
screenprint(char *fmt, ...)
{
	char buf[100];
	Point p;
	va_list args;

	va_start(args, fmt);
	doprint(buf, &buf[sizeof buf], fmt, args);
	va_end(args);
	p = add(screen.r.min, Pt(40, Dy(screen.r)-40));
	string(&screen, p, font, buf, S);
}


#define	Viewkey	0xb2

char *
getcmdstr(void)
{
	Event ev;
	int e;
	ulong timekey = 0;
	ulong tracktm = 0;
	Dir dir;

	if(track){
		timekey = etimer(0, 5000);
		if(dirstat(track, &dir) >= 0)
			tracktm = dir.mtime;
	}
	for (;;) {
		e = event(&ev);
		if ((e & Emouse) && ev.mouse.buttons) {
			mouse = ev.mouse;
			return getmousestr();
		} else if (e & Ekeyboard)
			return getkbdstr(ev.kbdc);	/* sadly, no way to unget */
		else if (e & timekey) {
			if((dirstat(track, &dir) >= 0) && (tracktm < dir.mtime))
				return "q\n";
		}
	}
}

char *
getkbdstr(int c0)
{
	static char buf[100];
	char *p;
	int c;

	if (c0 == '\n')
		return "";
	buf[0] = c0;
	buf[1] = 0;
	screenprint("%s", buf);
	for (p = buf+1; (c = ekbd()) != '\n' && c != '\r' && c != -1 && c != Viewkey; ) {
		if (c == '\b' && p > buf) {
			*--p = ' ';
		} else {
			*p++ = c;
			*p = 0;
		}
		screenprint("%s", buf);
	}
	*p = 0;
	return buf;
}


#define button3(b)	((b) & 4)
#define button2(b)	((b) & 2)
#define button1(b)	((b) & 1)
#define button23(b)	((b) & 6)
#define button123(b)	((b) & 7)

#define	butcvt(b)	(1 << ((b) - 1))

int buttondown(void)	/* report state of buttons, if any */
{
	if (!ecanmouse())	/* no event pending */
		return 0;
	mouse = emouse();	/* something, but it could be motion */
	return mouse.buttons & 7;
}

int waitdown(void)	/* wait until some button is down */
{
	while (!(mouse.buttons & 7))
		mouse = emouse();
	return mouse.buttons & 7;
}

int waitup(void)
{
	while (mouse.buttons & 7)
		mouse = emouse();
	return mouse.buttons & 7;
}

char *m3[]	= { "next", "prev", "page n", "again", "bigger", "smaller", "pan", "quit?", 0 };
char *m2[]	= { 0 };

enum { Next = 0, Prev, Page, Again, Bigger, Smaller, Pan, Quit };

Menu	mbut3	= { m3, 0, 0 };
Menu	mbut2	= { m2, 0, 0 };

int	last_hit;
int	last_but;

Point blackrect(int but, Rectangle r, Rectangle sr);

char *pan(int but)
{
	Rectangle r;
	Rectangle sr = Rect(0, 0, 2*85, 2*110);		/* tiny screen rectangle */
	Point dp;
	double xf, yf;

	cursorswitch(&blot);
	xf = Dx(screen.r) / 850.0;			/* should be params */
	yf = Dy(screen.r) / 1100.0;
	r = Rect(0, 0, Dx(sr)*xf, Dy(sr)*yf);
	/* print("in: xf,yf=%g,%g, r=%R, sr=%R\n", xf, yf, r, sr); /* */
	dp = blackrect(but, r, sr);

	/* needs a bailout test here */
	xf = (double) dp.x / (Dx(sr) - Dx(r));
	yf = (double) dp.y / (Dy(sr) - Dy(r));

	xyoffset.x = - xf * ( 850 - Dx(screen.r));
	xyoffset.y = - yf * (1100 - Dy(screen.r));

	/* print("screen %R, r %R, dp %P xf,yf %g,%g  xyoffset %P\n",
		screen.r, r, dp, xf, yf, xyoffset); /* */
	cursorswitch(0);
	return "p";
}

Point
blackrect(int but, Rectangle r, Rectangle sr)
{
	Point dp = Pt(0,0);
	Bitmap *bp;
	int b = butcvt(but);

	if (waitdown() != b) {
		waitup();
		return dp;
	}
	cursorset(add(screen.r.min, Pt(2,2)));
	raddp(r, Pt(2,2));
	raddp(sr, Pt(2,2));
	bp = balloc(r, screen.ldepth);
	border(&screen, raddp(sr, screen.r.min), 1, F&~D);	/* outline working area */
	bitblt(&screen, screen.r.min, bp, r, DxnorS);
	dp = screen.r.min;
	do {
		mouse = emouse();
		bitblt(&screen, add(r.min, dp), bp, r, DxnorS);
		dp = sub(mouse.xy, r.min);
		bitblt(&screen, add(r.min, dp), bp, r, DxnorS);
	} while (mouse.buttons & b);
	bitblt(&screen, add(r.min, dp), bp, r, DxnorS);
	bfree(bp);
	return sub(dp, screen.r.min);
}

char *getmousestr(void)
{
	static char buf[20];

	checkmouse();
	if (last_but == 2) {
		return "c";
	} else if (last_but == 3) {
		switch (last_hit) {
		case Next:
			return "";
		case Prev:
			return "-1";
		case Page:
			screenprint("page? ");
			return "c";
		case Again:
			return "p";
		case Bigger:
			sprint(buf, "m%g", mag * 1.1);
			return buf;
		case Smaller:
			sprint(buf, "m%g", mag / 1.1);
			return buf;
		case Pan:
			return pan(3);
		case Quit:
			return "q";
		default:
			return "c";
		}
	} else {		/* button 1 or bail out */
		return "c";
	}
}

int
checkmouse(void)	/* return button touched if any */
{
	int c, b;
	char *p;
	extern int confirm(int);

	b = waitdown();
	last_but = 0;
	last_hit = -1;
	c = 0;
	if (button3(b)) {
		last_hit = menuhit(3, &mouse, &mbut3);
		last_but = 3;
	} else if (button2(b)) {
		last_hit = menuhit(2, &mouse, &mbut2);
		last_but = 2;
	} else {		/* button1() */
		last_but = 1;
	}
	waitup();
	if (last_but == 3 && last_hit >= 0) {
		p = m3[last_hit];
		c = p[strlen(p) - 1];
	}
	if (c == '?' && !confirm(last_but))
		last_hit = -1;
	return last_but;
}

Cursor deadmouse = {
	{ 0, 0},	/* offset */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x0C, 0x00, 0x82, 0x04, 0x41,
	  0xFF, 0xE1, 0x5F, 0xF1, 0x3F, 0xFE, 0x17, 0xF0,
	  0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x0C, 0x00, 0x82, 0x04, 0x41,
	  0xFF, 0xE1, 0x5F, 0xF1, 0x3F, 0xFE, 0x17, 0xF0,
	  0x03, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor blot ={
	{ 0, 0 },
	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
	{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, }
};

Cursor skull ={
	{ 0, 0 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03,
	  0xE7, 0xE7, 0x3F, 0xFC, 0x0F, 0xF0, 0x0D, 0xB0,
	  0x07, 0xE0, 0x06, 0x60, 0x37, 0xEC, 0xE4, 0x27,
	  0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x03,
	  0xE7, 0xE7, 0x3F, 0xFC, 0x0F, 0xF0, 0x0D, 0xB0,
	  0x07, 0xE0, 0x06, 0x60, 0x37, 0xEC, 0xE4, 0x27,
	  0xC3, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

confirm(int but)	/* ask for confirmation if menu item ends with '?' */
{
	int c;
	static int but_cvt[8] = { 0, 1, 2, 0, 3, 0, 0, 0 };

	cursorswitch(&skull);
	c = waitdown();
	waitup();
	cursorswitch(0);
	return but == but_cvt[c];
}
