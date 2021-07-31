#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	<libg.h>
#include	<gnot.h>
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#include	"machine.h"
#include	"screen.h"

enum
{
	lcdmaxx=	768,
	lcdmaxy=	1024,
};

Lock screenlock;

int islittle = 1;		/* little endian bit ordering in bytes */

#define LCDLD	1
#define LCDBI2PIX	(1<<LCDLD)
#define LCDWORDS ((lcdmaxx*lcdmaxy*LCDBI2PIX)/BI2WD)
#define LCDLEN	(2+lcdmaxx)
#define LCDWID	(lcdmaxx*LCDBI2PIX/8)

static Rectangle NULLMBB = {10000, 10000, -10000, -10000};

#define	MINX	8

extern	GSubfont	defont0;
GSubfont		*defont;

struct
{
	Point	pos;
	int	bwid;
}out;

GBitmap	gscreen;
static Rectangle mbb;

static ulong	rep(ulong, int);

static lcddev = 4<<3;
static ulong swiz13[256];
Cursor fatarrow = {
	{ -1, -1 },
	{
		0xff, 0xff, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0c, 
		0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04, 
		0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8c, 0x04, 
		0x92, 0x08, 0x91, 0x10, 0xa0, 0xa0, 0xc0, 0x40, 
	},
	{
		0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xf0, 
		0x7f, 0xe0, 0x7f, 0xe0, 0x7f, 0xf0, 0x7f, 0xf8, 
		0x7f, 0xfc, 0x7f, 0xfe, 0x7f, 0xfc, 0x73, 0xf8, 
		0x61, 0xf0, 0x60, 0xe0, 0x40, 0x40, 0x00, 0x00, 
	},
};

void
mousescreenupdate(void)
{
	screenupdate();
}

void
lcdscsion(void)
{
	ulong x, m;

	for(x = 0; x < 256; x++) {
		m = (x&0xc0)<<18 | (x&0x30)<<12 | (x&0xc)<<6 | x&3;	/* should be right */
		swiz13[x] = (((m << 2) | m) >> 1) & 0x03030303;
	}
	mbb = NULLMBB;
}

void
screenupdate(void)
{
	ulong x, y, m;
	uchar *s;
	Rectangle r;
#ifdef NT
	ulong sx;
#endif

	r = mbb;
	mbb = NULLMBB;

	if(r.max.y <= r.min.y)
		return;

	r.min.y &= ~3;
	r.max.y = (r.max.y+31)&~31;	/* round up to word */
#ifdef NT
#define	bbuzz	for(i=0;i<10;i++)
	r.min.x &= ~3;
	if (r.min.x > 383)
		sx = r.min.x + 128;
	else
		sx = r.min.x;
#else
	r.min.x = 0;
#endif
	if(r.min.y < 0)
		r.min.y = 0;
	if(r.max.y > lcdmaxy)
		r.max.y = lcdmaxy;
	for (y = r.min.y; y < r.max.y; y += 4) {
		s = (uchar *) gaddr(&gscreen, Pt(r.min.x, y));
		LCDADDR->ctl = y >> 8;
		LCDADDR->ctl = (y >> 2) & 0x3f;
#ifdef NT
		bbuzz;
		LCDADDR->ctl = sx >> 6;
		bbuzz;
		LCDADDR->ctl = sx & 0x3f;
#endif
		for (x = r.min.x; x < r.max.x; x += 4, s++) {
			m = swiz13[s[0]];
			m |= swiz13[s[LCDWID]] << 2;
			m |= swiz13[s[2*LCDWID]] << 4;
			m |= swiz13[s[3*LCDWID]] << 6;
			m = ~m;
			LCDADDR->data = m;
			LCDADDR->data = m>>8;
			LCDADDR->data = m>>16;
			LCDADDR->data = m>>24;
		}
	}
}

void
mbbrect(Rectangle r)
{
	if (r.min.x < mbb.min.x)
		mbb.min.x = r.min.x;
	if (r.min.y < mbb.min.y)
		mbb.min.y = r.min.y;
	if (r.max.x > mbb.max.x)
		mbb.max.x = r.max.x;
	if (r.max.y > mbb.max.y)
		mbb.max.y = r.max.y;
}

void
mbbpt(Point p)
{
	if (p.x < mbb.min.x)
		mbb.min.x = p.x;
	if (p.y < mbb.min.y)
		mbb.min.y = p.y;
	if (p.x >= mbb.max.x)
		mbb.max.x = p.x+1;
	if (p.y >= mbb.max.y)
		mbb.max.y = p.y+1;
}

void
screeninit(int a)
{
	int i;
	ulong *l;
	extern Cursor arrow;
	extern void kbdinit(int);

	USED(a);

	kbdinit(2);
	eiaspecial(3, 0, &mouseq, 1200);
	conf.monitor = 1;

	memmove(&arrow, &fatarrow, sizeof(fatarrow));
	pixreverse(arrow.set, 2*16, 0);
	pixreverse(arrow.clr, 2*16, 0);

	gscreen.base = xalloc(2048 + LCDWORDS*BY2WD);
	gscreen.base += 512;
	gscreen.width = lcdmaxx*LCDBI2PIX/BI2WD;
	gscreen.ldepth = LCDLD;
	gscreen.r = Rect(0, 0, lcdmaxx, lcdmaxy);
	lcdscsion();

	/*
	 *  swizzle the font longs.  we do both byte and bit swizzling
	 *  since the font is initialized with big endian longs.
	 */
	defont = &defont0;
	l = defont->bits->base;
	for(i = defont->bits->width*Dy(defont->bits->r); i > 0; i--, l++)
		*l = (*l<<24) | ((*l>>8)&0x0000ff00) | ((*l<<8)&0x00ff0000) | (*l>>24);
	pixreverse((uchar*)defont->bits->base,
		defont->bits->width*BY2WD*Dy(defont->bits->r), 0);

	gscreen.clipr = gscreen.r;
	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, flipD[0]);
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;

	/*
	 * Hack - force loading of kernel bbmalloc.
	 */
	{
		extern int bbonstack(void);
		a = bbonstack();
		USED(a);
	}
}

void
screenputnl(void)
{
	Rectangle r;

	out.pos.x = MINX;
	out.pos.y += defont0.height;
	if(out.pos.y > gscreen.r.max.y-defont0.height)
		out.pos.y = gscreen.r.min.y;
	r = Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height);
	gbitblt(&gscreen, r.min, &gscreen, r, flipD[0]);
	mbbrect(r);
	screenupdate();
}

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];
	Rectangle rs;

	/* 
	 * Don't deadlock trying to print in interrupt.
	 */
	if((getcpureg(PSW) & PSW_IPL) == 0){
		if(!canlock(&screenlock))
			return;
	}
	else
		lock(&screenlock);

	rs.min = Pt(0, out.pos.y);
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
				gsubfstring(&gscreen, out.pos, defont, " ", flipD[S]);
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf, flipD[S]);
		}
	}
	rs.max = Pt(gscreen.r.max.x, out.pos.y+defont0.height);
	unlock(&screenlock);
	mbbrect(rs);
	screenupdate();
}

int
screenbits(void)
{
	return 8;				/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	ulong ans;

	if(p == 0)
		ans = ~0;
	else
		ans = 0;
	*pr = *pg = *pb = ans;
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p, r, g, b);
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
	mouseupdate(1);
}

void
mousectl(char *x)
{
	USED(x);
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
