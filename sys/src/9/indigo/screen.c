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
#undef	gbitblt
#undef	gtexture
#undef	gsegment
#undef	gpoint

#include	"lg1.h"

#define	XSCREEN	1024
#define	YSCREEN	768

#define	MINX	8

extern	GSubfont	defont0;
GSubfont		*defont;

struct
{
	Point	pos;
	int	bwid;
}out;

GBitmap gscreen = {
	0, 0, 0, 0,
	0, 0, XSCREEN, YSCREEN,
	0, 0, XSCREEN, YSCREEN,
	0
};

void	lg_blt(GBitmap*, Point, GBitmap*, Rectangle, Fcode);

static RGB rgbmap[256];

extern char	screenldepth[];

void
screeninit(int use)
{
	int i, ldepth = 3;

	USED(use);
	defont = &defont0;
	if(screenldepth[0])
		ldepth = strtol(screenldepth, 0, 10);
	if(ldepth != 0)
		ldepth = 3;
	i = BI2WD/(1<<ldepth);
	gscreen.width = (XSCREEN+i-1)/i;
	gscreen.base = xspanalloc(gscreen.width*BY2WD*YSCREEN,
		BY2PG, 128*1024*1024);

	gscreen.ldepth = 3;
	for(i=0; i<256; i++)
		setcolor(i, (((i>>5)&7)*255)/7.0,
			    (((i>>2)&7)*255)/7.0,
			    (( i    &3)*255)/3.0);
	setcolor(0x55, 0x55, 0x55, 0x55);
	setcolor(0xaa, 0xaa, 0xaa, 0xaa);
	gscreen.ldepth = ldepth;
	_gbitblt(&gscreen, gscreen.r.min, &gscreen, gscreen.r, F);
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
	_gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
		Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height),
		F);
}

Lock screenlock;

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];
	Point start;

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
				_gbitblt(&gscreen, out.pos, &gscreen, Rect(out.pos.x, out.pos.y, out.pos.x+out.bwid, out.pos.y+defont0.height), F);
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			start = out.pos;
			out.pos = gsubfstring(&gscreen, out.pos, &defont0, buf, notS);
			lg_blt(&gscreen, start, &gscreen, Rect(start.x, start.y, out.pos.x, out.pos.y+defont0.height), S);
		}
	}
	unlock(&screenlock);
}

int
screenbits(void)
{
	return 1<<gscreen.ldepth;	/* bits per pixel */
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	p &= 0xff;
	if(gscreen.ldepth == 0 && p != 0)
		p = 0xff;
	*pr = rgbmap[p].red;
	*pg = rgbmap[p].green;
	*pb = rgbmap[p].blue;
}

int 
setcolor(ulong c, ulong r, ulong g, ulong b)
{
	Rexchip *rex = REXADDR;

	c &= 0xff;
	if(gscreen.ldepth == 0 && c != 0)
		c = 0xff;
	r &= 0xff; r |= r<<8; r |= r<<16;
	g &= 0xff; g |= g<<8; g |= g<<16;
	b &= 0xff; b |= b<<8; b |= b<<16;
	rgbmap[c] = (RGB){r, g, b};

	REXWAIT;
	rex->p1.set.configsel = WRITE_ADDR;
	rex->p1.set.rwdac = 0x82;
	rex->p1.go.rwdac = 0x82;

	rex->p1.set.configsel = CONTROL;
	rex->p1.set.rwdac = 0x0f;
	rex->p1.go.rwdac = 0x0f;

	rex->p1.set.configsel = WRITE_ADDR;
	rex->p1.set.rwdac = c;
	rex->p1.go.rwdac = c;

	rex->p1.set.configsel = PALETTE_RAM;
	rex->p1.set.rwdac = r;
	rex->p1.go.rwdac = r;
	rex->p1.set.rwdac = g;
	rex->p1.go.rwdac = g;
	rex->p1.set.rwdac = b;
	rex->p1.go.rwdac = b;

	rex->p1.set.configsel = PIXEL_READ_MASK;
	rex->p1.set.rwdac = 0xff;
	rex->p1.go.rwdac = 0xff;

	return 1;
}

void
_gbitblt(GBitmap *dm, Point pt, GBitmap *sm, Rectangle r, Fcode fcode)
{
	gbitblt(dm, pt, sm, r, fcode);
	if(dm->base == gscreen.base){
		r.max.x = pt.x + r.max.x - r.min.x;
		r.max.y = pt.y + r.max.y - r.min.y;
		lg_blt(dm, pt, dm, Rpt(pt, r.max), fcode);
	}
}

void
_gtexture(GBitmap *dm, Rectangle r, GBitmap *sm, Fcode fcode)	
{
	gtexture(dm, r, sm, fcode);
	if(dm->base == gscreen.base)
		lg_blt(dm, r.min, dm, r, fcode);
}

void
_gpoint(GBitmap *b, Point p, int s, Fcode fcode)
{
	gpoint(b, p, s, fcode);
	if(b->base == gscreen.base)
		lg_blt(b, p, b, Rect(p.x, p.y, p.x+1, p.y+1), fcode);
}

void
_gsegment(GBitmap *b, Point p, Point q, int s, Fcode fcode)
{
	Rectangle r;

	gsegment(b, p, q, s, fcode);
	if(b->base == gscreen.base){
		r = rcanon(Rpt(p, q));
		lg_blt(b, r.min, b, r, S);
	}
}

void
hwscreenwrite(int miny, int maxy)
{
	lg_blt(&gscreen, Pt(gscreen.r.min.x, miny), &gscreen,
		Rect(gscreen.r.min.x, miny, gscreen.r.max.x, maxy), S);
}

void
lg_blt(GBitmap *dm, Point pt, GBitmap *sm, Rectangle r, Fcode fcode)
{
	Rexchip *rex = REXADDR;
	int width, height, xw, dw;
	ulong cmd, *rp, *xp;

	gbitbltclip(&dm);

	if((width = Dx(r)) <= 0)
		return;
	if((height = Dy(r)) <= 0)
		return;
	REXWAIT;
	rex->set.ystarti = pt.y;
	rex->set.xendi = pt.x + width - 1;
	rex->set.yendi = pt.y + height - 1;
	switch(fcode){
	case Zero:
		rex->set.xstarti = pt.x;
		rex->go.command = REX_LO_ZERO | REX_DRAW | QUADMODE |
				BLOCK | STOPONX | STOPONY;
		return;
	case notD:
		rex->set.xstarti = pt.x;
		rex->go.command = REX_LO_NDST | REX_DRAW | QUADMODE |
				BLOCK | STOPONX | STOPONY;
		return;
	case D:
		return;
	case F:
		rex->set.xstarti = pt.x;
		rex->go.command = REX_LO_ONE | REX_DRAW | QUADMODE |
				BLOCK | STOPONX | STOPONY;
		return;
	}
	if(dm->ldepth){
		rex->set.xstarti = pt.x & ~3;
		rex->go.command = REX_NOP;
		r.min.x &= ~3;
		width = (Dx(r)+3)/4;
		cmd = REX_LO_SRC | REX_DRAW | COLORAUX |
			QUADMODE | XYCONTINUE | BLOCK;
		rp = &rex->go.rwaux;
	}else{
		rex->set.colorback = 0;
		rex->set.colorredi = 255;
		rex->set.xstarti = pt.x & ~31;
		rex->go.command = REX_NOP;
		r.min.x &= ~31;
		width = (Dx(r)+31)/32;
		cmd = REX_LO_SRC | REX_DRAW | ENZPATTERN | ZOPAQUE |
			QUADMODE | STOPONX | XYCONTINUE | LENGTH32 | BLOCK;
		rp = &rex->go.zpattern;
	}
	xp = gaddr(sm, r.min);
	dw = sm->width - width;

	REXWAIT;
	rex->set.command = cmd;
	while(--height >= 0){
		xw = width;
		while(--xw >= 0)
			*rp = *xp++;
		xp += dw;
	}
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
	if(gscreen.base)
		mouseupdate(1);
}

/* only 1 flavor mouse */
void
mousectl(char *x)
{
	USED(x);
}
