#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

#define	MINX	8

extern	GSubfont	defont0;
GSubfont		*defont;


struct{
	Point	pos;
	int	bwid;
}out;

Lock	screenlock;

GBitmap	gscreen =
{
	(ulong*)((1*1024*1024)|KZERO),	/* bootrom puts it here; changed by mmuinit */
	0,
	64,
	0,
	{ 0, 0, 1024, 1024, },
	{ 0, 0, 1024, 1024, },
	0
};

void
screeninit(void)
{
	/*
	 * Read HEX switch to set ldepth
	 */
	if(*(uchar*)MOUSE & (1<<4))
		gscreen.ldepth = 1;
	defont = &defont0;	/* save space; let bitblt do the conversion work */
	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, 0);
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
	    Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height), 0);
}

void
screenputs(char *s, int n)
{
	Rune r;
	int i;
	char buf[4];

	if(getsr() & 0x0700){
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
				gsubfstring(&gscreen, out.pos, defont, " ", S);
			}
		}else{
			if(out.pos.x >= gscreen.r.max.x-out.bwid)
				screenputnl();
			out.pos = gsubfstring(&gscreen, out.pos, defont, buf, S);
		}
	}
	unlock(&screenlock);
}

int
screenbits(void)
{
	if(*(uchar*)MOUSE & (1<<4))
		return 2;
	else
		return 1;
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	ulong ans;

	/*
	 * The gnot says 0 is white (max intensity)
	 */
	if(gscreen.ldepth == 0){
		if(p == 0)
				ans = ~0;
		else
				ans = 0;
	}else{
		switch(p){
		case 0:		ans = ~0;		break;
		case 1:		ans = 0xAAAAAAAA;	break;
		case 2:		ans = 0x55555555;	break;
		default:	ans = 0;		break;
		}
	}
	*pr = *pg = *pb = ans;
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p, r, g, b);
	return 0;	/* can't change mono screen colormap */
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
mouseclock(void)	/* called spl6 */
{
	++mouse.clock;
}

void
mousetry(Ureg *ur)
{
	int s;

	if(mouse.clock && mouse.track && (ur->sr&SPL(7)) == 0 && canlock(&cursor)){
		s = spl1();
		mouseupdate(0);
		splx(s);
		unlock(&cursor);
		wakeup(&mouse.r);
	}
}

/* only 1 flavor mouse */
void
mousectl(char *x)
{
	USED(x);
}
