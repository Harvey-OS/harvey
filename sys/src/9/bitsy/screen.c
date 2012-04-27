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
#include "gamma.h"

#define	MINX	8

int landscape = 0;	/* orientation of the screen, default is 0: portait */

enum {
	Wid		= 240,
	Ht		= 320,
	Pal0	= 0x2000,	/* 16-bit pixel data in active mode (12 in passive) */

	hsw		= 0x00,
	elw		= 0x0e,
	blw		= 0x0d,

	vsw		= 0x02,
	efw		= 0x01,
	bfw		= 0x0a,

	pcd		= 0x10,
};

struct sa1110fb {
	/* Frame buffer for 16-bit active color */
	short	palette[16];		/* entry 0 set to Pal0, the rest to 0 */
	ushort	pixel[Wid*Ht];		/* Pixel data */
} *framebuf;

enum {
/* LCD Control Register 0, lcd->lccr0 */
	LEN	=  0,	/*  1 bit */
	CMS	=  1,	/*  1 bit */
	SDS	=  2,	/*  1 bit */
	LDM	=  3,	/*  1 bit */
	BAM	=  4,	/*  1 bit */
	ERM	=  5,	/*  1 bit */
	PAS	=  7,	/*  1 bit */
	BLE	=  8,	/*  1 bit */
	DPD	=  9,	/*  1 bit */
	PDD	= 12,	/*  8 bits */
};

enum {
/* LCD Control Register 1, lcd->lccr1 */
	PPL	=  0,	/* 10 bits */
	HSW	= 10,	/*  6 bits */
	ELW	= 16,	/*  8 bits */
	BLW	= 24,	/*  8 bits */
};

enum {
/* LCD Control Register 2, lcd->lccr2 */
	LPP	=  0,	/* 10 bits */
	VSW	= 10,	/*  6 bits */
	EFW	= 16,	/*  8 bits */
	BFW	= 24,	/*  8 bits */
};

enum {
/* LCD Control Register 3, lcd->lccr3 */
	PCD	=  0,	/*  8 bits */
	ACB	=  8,	/*  8 bits */
	API	= 16,	/*  4 bits */
	VSP	= 20,	/*  1 bit */
	HSP	= 21,	/*  1 bit */
	PCP	= 22,	/*  1 bit */
	OEP	= 23,	/*  1 bit */
};

enum {
/* LCD Status Register, lcd->lcsr */
	LDD	=  0,	/*  1 bit */
	BAU	=  1,	/*  1 bit */
	BER	=  2,	/*  1 bit */
	ABC	=  3,	/*  1 bit */
	IOL	=  4,	/*  1 bit */
	IUL	=  5,	/*  1 bit */
	OIU	=  6,	/*  1 bit */
	IUU	=  7,	/*  1 bit */
	OOL	=  8,	/*  1 bit */
	OUL	=  9,	/*  1 bit */
	OOU	= 10,	/*  1 bit */
	OUU	= 11,	/*  1 bit */
};

struct sa1110regs {
	ulong	lccr0;
	ulong	lcsr;
	ulong	dummies[2];
	short*	dbar1;
	ulong	dcar1;
	ulong	dbar2;
	ulong	dcar2;
	ulong	lccr1;
	ulong	lccr2;
	ulong	lccr3;
} *lcd;

Point	ZP = {0, 0};

static Memdata xgdata;

static Memimage xgscreen =
{
	{ 0, 0, Wid, Ht },		/* r */
	{ 0, 0, Wid, Ht },		/* clipr */
	16,				/* depth */
	3,				/* nchan */
	RGB16,			/* chan */
	nil,				/* cmap */
	&xgdata,			/* data */
	0,				/* zero */
	Wid/2,			/* width */
	0,				/* layer */
	0,				/* flags */
};

struct{
	Point	pos;
	int	bwid;
}out;

Memimage *gscreen;
Memimage *conscol;
Memimage *back;

Memsubfont	*memdefont;

Lock		screenlock;

Point		ZP = {0, 0};
ushort		*vscreen;	/* virtual screen */
Rectangle	window;
Point		curpos;
int			h, w;
int			drawdebug;

static	ulong	rep(ulong, int);
static	void	screenwin(void);
static	void	screenputc(char *buf);
static	void	bitsyscreenputs(char *s, int n);
static	void	scroll(void);

static void
lcdinit(void)
{
	/* the following line works because main memory is direct mapped */
	gpioregs->direction |= 
		GPIO_LDD8_o|GPIO_LDD9_o|GPIO_LDD10_o|GPIO_LDD11_o
		|GPIO_LDD12_o|GPIO_LDD13_o|GPIO_LDD14_o|GPIO_LDD15_o;
	gpioregs->altfunc |= 
		GPIO_LDD8_o|GPIO_LDD9_o|GPIO_LDD10_o|GPIO_LDD11_o
		|GPIO_LDD12_o|GPIO_LDD13_o|GPIO_LDD14_o|GPIO_LDD15_o;
	framebuf->palette[0] = Pal0;
	lcd->dbar1 = framebuf->palette;
	lcd->lccr3 = pcd<<PCD | 0<<ACB | 0<<API | 1<<VSP | 1<<HSP | 0<<PCP | 0<<OEP;
	lcd->lccr2 = (Wid-1)<<LPP | vsw<<VSW | efw<<EFW | bfw<<BFW;
	lcd->lccr1 = (Ht-16)<<PPL | hsw<<HSW | elw<<ELW | blw<<BLW;
	lcd->lccr0 = 1<<LEN | 0<<CMS | 0<<SDS | 1<<LDM | 1<<BAM | 1<<ERM | 1<<PAS | 0<<BLE | 0<<DPD | 0<<PDD;
}

void
flipscreen(int ls) {
	if (ls == landscape)
		return;
	if (ls) {
		gscreen->r = Rect(0, 0, Ht, Wid);
		gscreen->clipr = gscreen->r;
		gscreen->width = Ht/2;
		xgdata.bdata = (uchar *)framebuf->pixel;
	} else {
		gscreen->r = Rect(0, 0, Wid, Ht);
		gscreen->clipr = gscreen->r;
		gscreen->width = Wid/2;
		xgdata.bdata = (uchar *)vscreen;
	}
	landscape = ls;
}

void
lcdtweak(Cmdbuf *cmd)
{
	if(cmd->nf < 4)
		return;
	if(*cmd->f[0] == 'h')
		lcd->lccr1 = ((Ht-16)<<PPL)
			| (atoi(cmd->f[1])<<HSW)
			| (atoi(cmd->f[2])<<ELW)
			| (atoi(cmd->f[3])<<BLW);
	if(*cmd->f[0] == 'v')
		lcd->lccr2 = ((Wid-1)<<LPP)
			| (atoi(cmd->f[1])<<VSW)
			| (atoi(cmd->f[2])<<EFW)
			| (atoi(cmd->f[3])<<BFW);
}

void
screenpower(int on)
{
	blankscreen(on == 0);
}

void
screeninit(void)
{
	int i;

	/* map the lcd regs into the kernel's virtual space */
	lcd = (struct sa1110regs*)mapspecial(LCDREGS, sizeof(struct sa1110regs));;

	framebuf = xspanalloc(sizeof *framebuf, 0x100, 0);

	vscreen = xalloc(sizeof(ushort)*Wid*Ht);

	lcdpower(1);
	lcdinit();

	gscreen = &xgscreen;

	xgdata.ref = 1;
	i = 0;
	if (landscape) {
		gscreen->r = Rect(0, 0, Ht, Wid);
		gscreen->clipr = gscreen->r;
		gscreen->width = Ht/2;
		xgdata.bdata = (uchar *)framebuf->pixel;
		while (i < Wid*Ht*1/3)	framebuf->pixel[i++] = 0xf800;	/* red */
		while (i < Wid*Ht*2/3)	framebuf->pixel[i++] = 0xffff;		/* white */
		while (i < Wid*Ht*3/3)	framebuf->pixel[i++] = 0x001f;	/* blue */
	} else {
		gscreen->r = Rect(0, 0, Wid, Ht);
		gscreen->clipr = gscreen->r;
		gscreen->width = Wid/2;
		xgdata.bdata = (uchar *)vscreen;
		while (i < Wid*Ht*1/3)	vscreen[i++] = 0xf800;	/* red */
		while (i < Wid*Ht*2/3)	vscreen[i++] = 0xffff;	/* white */
		while (i < Wid*Ht*3/3)	vscreen[i++] = 0x001f;	/* blue */
		flushmemscreen(gscreen->r);
	}
	memimageinit();
	memdefont = getmemdefont();

	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = memdefont->info[' '].width;

	blanktime = 3;	/* minutes */

	screenwin();
//	screenputs = bitsyscreenputs;
	screenputs = nil;
}

void
flushmemscreen(Rectangle r)
{
	int x, y;
	ulong start, end;

	if (landscape == 0) {
		if (r.min.x <   0) r.min.x = 0;
		if (r.max.x > Wid) r.max.x = Wid;
		if (r.min.y <   0) r.min.y = 0;
		if (r.max.y >  Ht) r.max.y = Ht;
		for (x = r.min.x; x < r.max.x; x++)
			for (y = r.min.y; y < r.max.y; y++)
				framebuf->pixel[(x+1)*Ht-1-y] = gamma[vscreen[y*Wid+x]];
		start = (ulong)&framebuf->pixel[(r.min.x+1)*Ht-1-(r.max.y-1)];
		end = (ulong)&framebuf->pixel[(r.max.x-1+1)*Ht-1-(r.min.y)];
	} else {
		start = (ulong)&framebuf->pixel[r.min.y*Ht + r.min.x];
		end = (ulong)&framebuf->pixel[(r.max.y-1)*Ht + r.max.x - 1];
	}
	cachewbregion(start, end-start);
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
	*softscreen = (landscape == 0);

	return (uchar*)gscreen->data->bdata;
}

void
getcolor(ulong p, ulong* pr, ulong* pg, ulong* pb)
{
	USED(p, pr, pg, pb);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p,r,g,b);
	return 0;
}

void
blankscreen(int blank)
{
	int cnt;

	if (blank) {
		lcd->lccr0 &= ~(1<<LEN);	/* disable the LCD */
		cnt = 0;
		while((lcd->lcsr & (1<<LDD)) == 0) {
			delay(10);
			if (++cnt == 100) {
				iprint("LCD doesn't stop\n");
				break;
			}
		}
		lcdpower(0);
	} else {
		lcdpower(1);
		lcdinit();
	}
}

static void
bitsyscreenputs(char *s, int n)
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
	Point p, q;
	char *greet;
	Memimage *orange;
	Rectangle r;

	memsetchan(gscreen, RGB16);

	back = memwhite;
	conscol = memblack;

	orange = allocmemimage(Rect(0,0,1,1), RGB16);
	orange->flags |= Frepl;
	orange->clipr = gscreen->r;
	orange->data->bdata[0] = 0x40;
	orange->data->bdata[1] = 0xfd;

	w = memdefont->info[' '].width;
	h = memdefont->height;

	r = insetrect(gscreen->r, 4);

	memimagedraw(gscreen, r, memblack, ZP, memopaque, ZP, S);
	window = insetrect(r, 4);
	memimagedraw(gscreen, window, memwhite, ZP, memopaque, ZP, S);

	memimagedraw(gscreen, Rect(window.min.x, window.min.y,
			window.max.x, window.min.y+h+5+6), orange, ZP, nil, ZP, S);
	freememimage(orange);
	window = insetrect(window, 5);

	greet = " Plan 9 Console ";
	p = addpt(window.min, Pt(10, 0));
	q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	flushmemscreen(r);
	window.min.y += h+6;
	curpos = window.min;
	window.max.y = window.min.y+((window.max.y-window.min.y)/h)*h;
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
		if(curpos.x >= window.max.x-4*w)
			screenputc("\n");

		pos = (curpos.x-window.min.x)/w;
		pos = 4-(pos%4);
		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+pos*w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		flushmemscreen(r);
		curpos.x += pos*w;
		break;
	case '\b':
		if(xp <= xbuf)
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

		if(curpos.x >= window.max.x-w)
			screenputc("\n");

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		flushmemscreen(r);
		curpos.x += w;
	}
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
	flushmemscreen(r);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP, S);
	flushmemscreen(r);

	curpos.y -= o;
}
