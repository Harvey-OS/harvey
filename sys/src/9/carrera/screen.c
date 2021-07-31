#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"screen.h"

#define	MINX	8

#define DAC	((Dac*)BTDac)
typedef struct Dac Dac;
struct Dac
{
	uchar	pad0[7];
	uchar	cr0;
	uchar	pad1[7];
	uchar	cr1;
	uchar	pad2[7];
	uchar	cr2;
	uchar	pad3[7];
	uchar	cr3;
};

char s1[] = { 0x00, 0x00, 0xC0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#define	Backgnd		0xFF	/* white */

		Memsubfont	*memdefont;
static	ulong		rep(ulong, int);

struct{
	Point	pos;
	int	bwid;
}out;

Lock	screenlock;

Point	ZP = {0, 0};

static Memdata gscreendata =
{
	nil,
	(ulong*)(Screenvirt+0x000178D0),
	1,
};

static Memimage xgscreen =
{
	{ 0, 0, 1597, 1234 },	/* r */
	{ 0, 0, 1597, 1234 },	/* clipr */
	8,	/* depth */
	1,	/* nchan */
	CMAP8,	/* chan */
	nil,	/* cmap */
	&gscreendata,	/* data */
	0,	/* zero */
	512,	/* width */
	0,	/* layer */
	0,	/* flags */
};

Memimage *gscreen;
Memimage *conscol;
Memimage *back;
Memimage *hwcursor;

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

static Rectangle window;
static Point curpos;
static int h, w;
int drawdebug;

void
screenwin(void)
{
	Dac *d;
	Point p, q;
	Cursor zero;
	char *greet;
	Memimage *grey;

	gscreen = &xgscreen;
	memsetchan(gscreen, CMAP8);
	hwcursor = allocmemimage(Rect(0,0,64,64), GREY2);
	memsetchan(hwcursor, GREY2);
	
	back = memwhite;
	conscol = memblack;

	memset((void*)Screenvirt, 0, 3*1024*1024);
	memfillcolor(gscreen, 0x444488FF);

	w = memdefont->info[' '].width;
	h = memdefont->height;

	window.min = Pt(100, 100);
	window.max = addpt(window.min, Pt(10+w*120, 10+h*60));

	memimagedraw(gscreen, window, memblack, ZP, memopaque, ZP);
	window = insetrect(window, 4);
	memimagedraw(gscreen, window, memwhite, ZP, memopaque, ZP);

	/* a lot of work to get a grey color */
	grey = allocmemimage(Rect(0,0,1,1), CMAP8);
	grey->flags |= Frepl;
	grey->clipr = gscreen->r;
	grey->data->bdata[0] = 0xAA;
	memimagedraw(gscreen, Rect(window.min.x, window.min.y,
			window.max.x, window.min.y+h+5+6), grey, ZP, nil, ZP);
	freememimage(grey);
	window = insetrect(window, 5);

	greet = " Plan 9 Console ";
	p = addpt(window.min, Pt(10, 0));
	q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	window.min.y += h+6;
	curpos = window.min;
	window.max.y = window.min.y+((window.max.y-window.min.y)/h)*h;

	d = DAC;
	/* cursor color 1: white */
	d->cr1 = 0x01;
	d->cr0 = 0x81;
	d->cr2 = 0xFF;
	d->cr2 = 0xFF;
	d->cr2 = 0xFF;
	/* cursor color 2: noir */
	d->cr1 = 0x01;
	d->cr0 = 0x82;
	d->cr2 = 0;
	d->cr2 = 0;
	d->cr2 = 0;
	/* cursor color 3: schwarz */
	d->cr1 = 0x01;
	d->cr0 = 0x83;
	d->cr2 = 0;
	d->cr2 = 0;
	d->cr2 = 0;
	/* initialize with all-transparent cursor */
	memset(&zero, 0, sizeof zero);
	setcursor(&zero);
	/* enable both planes of cursor */
	d->cr1 = 0x03;
	d->cr0 = 0x00;
	d->cr2 = 0xc0;
}

void
dacinit(void)
{
	Dac *d;
	int i;

	d = DAC;

	/* Control registers */
	d->cr0 = 0x01;
	d->cr1 = 0x02;
	for(i = 0; i < sizeof s1; i++)
		d->cr2 = s1[i];

	/* Cursor programming */
	d->cr0 = 0x00;
	d->cr1 = 0x03;
	d->cr2 = 0xC0;
	for(i = 0; i < 12; i++)
		d->cr2 = 0;

	/* Load Cursor Ram */
	d->cr0 = 0x00;
	d->cr1 = 0x04;
	for(i = 0; i < 0x400; i++)
		d->cr2 = 0xff;

	drawcmap();

	/* Overlay Palette Ram */
	d->cr0 = 0x00;
	d->cr1 = 0x01;
	for(i = 0; i < 0x10; i++) {
		d->cr2 = 0xff;
		d->cr2 = 0xff;
		d->cr2 = 0xff;
	}

	/* Overlay Palette Ram */
	d->cr0 = 0x81;
	d->cr1 = 0x01;
	for(i = 0; i < 3; i++) {
		d->cr2 = 0xff;
		d->cr2 = 0xff;
		d->cr2 = 0xff;
	}
}

void
screeninit(void)
{
	dacinit();

	memimageinit();
	memdefont = getmemdefont();

	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = memdefont->info[' '].width;

	screenwin();
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
	memimagedraw(gscreen, r, gscreen, p, nil, p);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP);

	curpos.y -= o;
}

/* 
 * export screen to devdraw
 */
uchar*
attachscreen(Rectangle *r, ulong *chan, int* d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 0;

	return gscreendata.bdata;
}

/*
 * update screen (no-op: frame buffer is direct-mapped)
 */
void
flushmemscreen(Rectangle r)
{
	USED(r);
/*
	DEBUG version flashes the screen to indicate what's being painted
	Memimage *x;
	int i;

	if(Dx(r)<=0 || Dy(r)<=0)
		return;
	x = allocmemimage(r, 3);
	memimagedraw(x, r, &gscreen, r.min, memopaque, r.min);
	memimagedraw(&gscreen, r, memblack, r.min, memopaque, r.min);
	for(i=1000000; --i>0; );
	memimagedraw(&gscreen, r, x, r.min, memopaque, r.min);
	freememimage(x);
*/
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
		if(curpos.x >= window.max.x-8*w)
			screenputc("\n");

		pos = (curpos.x-window.min.x)/w;
		pos = 8-(pos%8);
		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+pos*w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min);
		curpos.x += pos*w;
		break;
	case '\b':
		if(xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min);
		curpos.x = *xp;
		break;
	case '\0':
		break;
	default:
		p = memsubfontwidth(memdefont, buf);
		w = p.x;

		if(curpos.x >= window.max.x-w)
			screenputc("\n");

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		curpos.x += w;
	}
}

void
screenputs(char *s, int n)
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

uchar revtab0[] = {
 0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
 0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
 0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
 0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
 0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
 0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	Dac *d;
	uchar r, g, b;
	extern uchar revtab0[];

	d = DAC;

	/* to protect DAC from mouse movement */
	lock(&cursor);

	d->cr0 = revtab0[p & 0xFF];
	d->cr1 = 0;
	r = d->cr3;
	g = d->cr3;
	b = d->cr3;
	*pr = (r<<24) | (r<<16) | (r<<8) | r;
	*pg = (g<<24) | (g<<16) | (g<<8) | g;
	*pb = (b<<24) | (b<<16) | (b<<8) | b;

	unlock(&cursor);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	Dac *d;
	extern uchar revtab0[];

	d = DAC;

	/* to protect DAC from mouse movement */
	lock(&cursor);

	d->cr0 = revtab0[p & 0xFF];
	d->cr1 = 0;
	d->cr3 = r >> 24;
	d->cr3 = g >> 24;
	d->cr3 = b >> 24;

	unlock(&cursor);

	return 1;
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

void
setcursor(Cursor *curs)
{
	Dac *d;
	int x, y, i;
	Point org;
	uchar ylow, yhigh, *p;
	ulong spix, cpix, dpix;
	ushort s[16], c[16];

	for(i=0; i<16; i++){
		p = (uchar*)&s[i];
		*p = curs->set[2*i];
		*(p+1) = curs->set[2*i+1];
		p = (uchar*)&c[i];
		*p = curs->clr[2*i];
		*(p+1) = curs->clr[2*i+1];
	}
	memset(hwcursor->data->bdata, 0, sizeof(long)*hwcursor->width*Dy(hwcursor->r));
	/* hw cursor is 64x64 with hot point at (32,32) */
	org = addpt(Pt(32,32), curs->offset); 
	for(y = 0; y < 16; y++)
		for(x = 0; x < 16; x++) {
			spix = (s[y]>>(15-x))&1;
			cpix = (c[y]>>(15-x))&1;
			dpix = (spix<<1) | cpix;
			/* point(&hwcursor, addpt(Pt(x,y), org), dpix, S), by hand */
			*byteaddr(hwcursor, Pt(x+org.x, y+org.y)) |= (dpix<<(6-(2*((x+org.x)&3))));
		}

	d = DAC;

	/* have to set y offscreen before writing cursor bits */
	d->cr1 = 0x03;
	d->cr0 = 0x03;
	ylow = d->cr2;
	yhigh = d->cr2;
	d->cr1 = 0x03;
	d->cr0 = 0x03;
	d->cr2 = 0xFF;
	d->cr2 = 0xFF;
	/* now set the bits */
	d->cr1 = 0x04;
	d->cr0 = 0x00;
	for(x = 0; x < 1024; x++)
		d->cr2 = hwcursor->data->bdata[x];
	/* set y back */
	d->cr1 = 0x03;
	d->cr0 = 0x03;
	d->cr2 = ylow;
	d->cr2 = yhigh;
}

int
cursoron(int dolock)
{
	Dac *d;
	Point p;

	p = mousexy();

	d = DAC;

	if(dolock)
		lock(&cursor);

	p.x += 296;		/* adjusted by experiment */
	p.y += 9;		/* adjusted by experiment */
	d->cr1 = 03;
	d->cr0 = 01;
	d->cr2 = p.x&0xFF;
	d->cr2 = (p.x>>8)&0xF;
	d->cr2 = p.y&0xFF;
	d->cr2 = (p.y>>8)&0xF;

	if(dolock)
		unlock(&cursor);
	return 0;
}

int
screenbits(void)
{
	return gscreen->depth;
}

extern	cursorlock(Rectangle);
extern	cursorunlock(void);

/*
 * paste tile into screen.
 * tile is at location r, first pixel in *data. 
 * tl is length of scan line to insert,
 * l is amount to advance data after each scan line.
 * gscreen.ldepth is known to be >= 3
 */
void
screenload(Rectangle r, uchar *data, int tl, int l, int dolock)
{
	uchar *q;
	int y;

	USED(dolock);

	if(!rectclip(&r, gscreen->r) || tl<=0)
		return;

	lock(&screenlock);
	q = byteaddr(gscreen, r.min);
	for(y=r.min.y; y<r.max.y; y++){
		memmove(q, data, tl);
		q += gscreen->width*sizeof(ulong);
		data += l;
	}
	unlock(&screenlock);
}

/*
 * get a tile from screen memory.
 * tile is at location r, first pixel in *data. 
 * tl is length of scan line to insert,
 * l is amount to advance data after each scan line.
 * gscreen.ldepth is known to be >= 3.
 * screenunload() doesn't clip, so must be
 * called correctly.
 */
void
screenunload(Rectangle r, uchar *data, int tl, int l, int dolock)
{
	uchar *q;
	int y;

	USED(dolock);

	lock(&screenlock);
	q = byteaddr(gscreen, r.min);
	for(y=r.min.y; y<r.max.y; y++){
		memmove(data, q, tl);
		q += gscreen->width*sizeof(ulong);
		data += l;
	}
	unlock(&screenlock);
}

void
blankscreen(int)
{
}
