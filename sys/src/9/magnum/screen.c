#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

enum
{
	monomaxx=	1152,
	monomaxy=	900,
	colmaxx=	2048,
	colmaxxvis=	1280,
	colmaxy=	1024,
	colrepl=	  16,	/* each scanline is repeated this many times */
};

#define MONOWORDS ((monomaxx*monomaxy)/BI2WD)

#define	MINX	8

extern	GSubfont	defont0;
GSubfont		*defont;

struct
{
	Point	pos;
	int	bwid;
}out;

GBitmap	gscreen;

enum {
	Mono = 1,
	Color = 2,
};


static int	scr;			/* Mono or Color or Not at all*/

static ulong	rep(ulong, int);
static int	iscolor(void);

static void
monodmaoff(DMAdev *dev)
{
	/*
	 *  turn off DMA, purge FIFO
	 */
	dev->mode = 0;
	while(dev->mode & Enable)
		delay(100);
	dev->mode = 1<<31;
	while(dev->mode & 0xFF)
		delay(100);
}

static void
monodmaon(DMAdev *dev, ulong mono)
{
	/*
	 *  turn on DMA to screen
	 */
	dev->laddr = mono;
	dev->block = MONOWORDS/16;
	delay(100);
	dev->mode = Enable | Autoreload;
}

void
screeninit(int use)
{

	int i;
	ulong r, g, b;
	DMAdev *dev = DMA2;
	ulong mono;

	monodmaoff(dev);
	switch(use){
	case '0':
	case '1':
		scr = 0;
		break;
	case 'm':
		scr = Mono;
		break;
	default:
		scr = iscolor() ? Color : Mono;
		break;
	}

	if(scr == Mono){
		mono = (ulong)xspanalloc(MONOWORDS*BY2WD, BY2PG, 512*1024);
		mono &= ~KZERO;
		gscreen.base = (ulong*)(UNCACHED|mono);
		gscreen.width = monomaxx/BI2WD;
		gscreen.ldepth = 0;
		gscreen.r = Rect(0, 0, monomaxx, monomaxy);
		monodmaon(dev, mono);
	}
	else
	if(scr == Color){
		gscreen.base = COLORFB;
		gscreen.width = colmaxx*colrepl/BY2WD;
		gscreen.ldepth = 3;
		gscreen.r = Rect(0, 0, colmaxxvis, colmaxy);
		/*
		 * For now, just use a fixed colormap, where pixel i is
		 * regarded as 3 bits of red, 3 bits of green, and 2 bits of blue.
		 * Intensities are inverted so that 0 means white, 255 means black.
		 * Exception: pixels 85 and 170 are set to intermediate grey values
		 * so that 2-bit grey scale images will look ok on this screen.
		 */
		for(i = 0; i<256; i++) {
			r = ~rep((i>>5) & 7, 3);
			g = ~rep((i>>2) & 7, 3);
			b = ~rep(i & 3, 2);
			setcolor(i, r, g, b);
		}
		setcolor(85, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);
		setcolor(170, 0x55555555, 0x55555555, 0x55555555);
	}
	else {
		mono = (ulong)xspanalloc(BY2PG, BY2PG, 0);
		mono &= ~KZERO;
		gscreen.base = (ulong*)(UNCACHED|mono);
		gscreen.width = BI2WD/BI2WD;
		gscreen.ldepth = 0;
		gscreen.r = Rect(0, 0, BI2WD, 32);
	}
	defont = &defont0;	/* save space; let bitblt do the conversion work */
	gscreen.clipr = gscreen.r;
	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, scr==Mono ? flipD[0] : 0);
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;
}

void
screenputnl(void)
{
	out.pos.x = MINX;
	out.pos.y += defont0.height;
	if(out.pos.y > gscreen.r.max.y-defont0.height)
		out.pos.y = gscreen.r.min.y;
	gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
	    Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height),
			scr==Mono ? flipD[0] : 0);
}

Lock screenlock;

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];

	if(scr == 0)
		return;

	if((getstatus() & IEC) == 0){
		if(!canlock(&screenlock))
			return;	/* don't deadlock trying to print in interrupt */
	}else
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
		if(r == '\n')
			screenputnl();
		else if(r == '\t'){
			out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
			if(out.pos.x >= gscreen.r.max.x)
				screenputnl();
		}else if(r == '\b'){
			if(out.pos.x >= out.bwid+MINX){
				out.pos.x -= out.bwid;
				gsubfstring(&gscreen, out.pos, defont, " ",
					scr==Mono? flipD[S] : S);
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf,
					scr==Mono? flipD[S] : S);
		}
	}
	unlock(&screenlock);
}

int
screenbits(void)
{
	return scr==Mono? 1 : 8;	/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	uchar r, g, b;
	uchar *kr = COLORKREG;
	ulong ans;

	if(scr == Mono) {
		/*
		 * The magnum monochrome says 0 is black (zero intensity)
		 */
		if(p == 0)
			ans = 0;
		else
			ans = ~0;
		*pr = *pg = *pb = ans;
	} else {
		/* not reliable unless done while vertical blanking */
		while(!(*kr & 0x20))
			continue;
		*COLORADDRH = 0;
		*COLORADDRL = p & 0xFF;
		r = *COLORPALETTE;
		g = *COLORPALETTE;
		b = *COLORPALETTE;
		*pr = (r<<24) | (r<<16) | (r<<8) | r;
		*pg = (g<<24) | (g<<16) | (g<<8) | g;
		*pb = (b<<24) | (b<<16) | (b<<8) | b;
	}
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	uchar *kr = COLORKREG;

	if(scr == Mono)
		return 0;	/* can't change mono screen colormap */
	else {
		/* not reliable unless done while vertical blanking */
		while(!(*kr & 0x20))
			continue;
		*COLORADDRH = 0;
		*COLORADDRL = p & 0xFF;
		*COLORPALETTE = r >> 24;
		*COLORPALETTE = g >> 24;
		*COLORPALETTE = b >> 24;
		return 1;
	}
}

/*
 *  look at blanking for a change
 */
static int
iscolor(void)
{
	uchar *kr = COLORKREG;
	int i, first;

	first = *kr & 0x30;
	for(i = 0; i < 10000; i++)
		if((*kr & 0x30) != first)
			return 1;
	return 0;
}

int
hwcursset(uchar *s, uchar *c, int ox, int oy)
{
	USED(s, c, ox, oy);
	return 0;
}

int
hwcursmove(int x, int y)
{
	USED(x, y);
	return 0;
}

void
mouseclock(void)	/* called splhi */
{
	if(scr)
		mouseupdate(1);
}

/* replicate (from top) value in v (n bits) until it fills a ulong */
static ulong
rep(ulong v, int n)
{
	int o;
	ulong rv;

	rv = 0;
	for(o = 32 - n; o >= 0; o -= n)
		rv |= (v << o);
	return rv;
}
