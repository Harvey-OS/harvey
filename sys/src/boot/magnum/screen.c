#include	"all.h"
#include	<libg.h>
#include	<gnot.h>

Point	gchar(GBitmap*, Point, GFont*, int, Fcode);

enum {
	Mono		= 1,
	Color		= 2,
	Monomaxx	= 1152,
	Monomaxy	= 900,
	Colmaxx		= 2048,
	Colmaxxvis	= 1280,
	Colmaxy		= 1024,
	Colrepl		= 16,	/* each scanline is repeated this many times */
};

#define MONOWORDS ((Monomaxx*Monomaxy)/BI2WD)
#define MONOPAGES ((MONOWORDS*BY2WD+BY2PG-1)/BY2PG)
#define	TOP	(8*1024*1024)
ulong	*mono;

#define	MINX	8

extern	GSubfont	defont0;

struct{
	Point	pos;
	int	bwid;
}out;

GBitmap	gscreen;
IOQ	printq;
int	istty;
static int	scr;			/* Mono or Color */

/*
 * Some monochrome screens are reversed from what we like:
 * We want 0's bright and 1s dark.
 * Indexed by an Fcode, these compensate for the source bitmap being wrong
 * (exchange S rows) and destination(exchange D columns and invert result)
 */
int flipD[] ={
	0xF, 0xD, 0xE, 0xC, 0x7, 0x5, 0x6, 0x4,
	0xB, 0x9, 0xA, 0x8, 0x3, 0x1, 0x2, 0x0, 
};

void
ttyscreen(void)
{
	/*
	 * kdbinit already inited kbdq
	 */
	istty = 1;
	initq(&printq);
	sccsetup(SCCADDR, SCCFREQ);
	sccspecial(1, &printq, &kbdq, 9600);
}

/*
 *  Set up the DMA to map the screen bits somewhere
 */
void
screeninit(int use)
{
	DMAdev *dev = DMA2;

	if(istty)
		return;
	scr = (use == 'c') ? Color : Mono;
	if(scr == Mono){
		/*
		 *  use top of memory, so dma does not cross 512K byte boundary
		 */
		mono = (ulong *)(TOP - MONOPAGES*BY2PG);
		gscreen.ldepth = 0;
		gscreen.base = (ulong*)(UNCACHED|(ulong)mono);
		gscreen.width = Monomaxx/BI2WD;
		gscreen.r = Rect(0, 0, Monomaxx, Monomaxy);

		/*
		 *  turn off DMA, purge FIFO
		 */
		dev->mode = 0;
		while(dev->mode & Enable)
			delay(100);
		dev->mode = 1<<31;
		while(dev->mode & 0xFF)
			delay(100);

		/*
		 *  turn on DMA to screen
		 */
		dev->laddr = (ulong)mono;
		dev->block = MONOWORDS/16;
		delay(100);
		dev->mode = Enable | Autoreload;
	}else{
		mono = (ulong*)(TOP-BY2PG);
		gscreen.base = COLORFB;
		gscreen.width = Colmaxx * Colrepl/BY2WD;
		gscreen.ldepth = 3;
		gscreen.r = Rect(0, 0, Colmaxxvis, Colmaxy);
		/*
		 * For now, just use a fixed colormap:
		 * 0 == white and 255 == black
		 */
		setcolor(0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
		setcolor(255, 0x00000000, 0x00000000, 0x00000000);
	}

	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, scr==Mono ? flipD[0] : 0);
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;
}

void
screenputc(int c)
{
	Fontchar *i;
	Point p;

	if(istty){
		sccputc(&printq, c);
		return;
	}
	switch(c){
	case '\n':
		out.pos.x = MINX;
		out.pos.y += defont0.height;
		if(out.pos.y > gscreen.r.max.y-defont0.height)
			out.pos.y = gscreen.r.min.y;
		gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
		  Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height),
		  scr==Mono ? flipD[0] : 0);
		break;
	case '\t':
		out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
		if(out.pos.x >= gscreen.r.max.x)
			screenputc('\n');
		break;
	case '\b':
		if(out.pos.x >= out.bwid+MINX){
			out.pos.x -= out.bwid;
			screenputc(' ');
			out.pos.x -= out.bwid;
		}
		break;
	default:
		if(out.pos.x >= gscreen.r.max.x-out.bwid)
			screenputc('\n');
		c &= 0x7f;
		if(c <= 0 || c >= defont0.n)
			break;
		i = defont0.info + c;
		p = out.pos;
		gbitblt(&gscreen, Pt(p.x+i->left, p.y), defont0.bits,
			Rect(i[0].x, 0, i[1].x, defont0.height),
			scr==Mono ? flipD[S] : S);
		out.pos.x = p.x + i->width;
		break;
	}
}

void
screenputs(char *s, int n)
{
	while(n-- > 0)
		screenputc(*s++);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	uchar *kr = COLORKREG;

	if(scr == Mono)
		return 0;	/* can't change mono screen colormap */
	else{
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

static ulong	addr;

void*
ialloc(ulong n, int align)
{
	ulong p;

	if(addr == 0)
		addr = ((ulong)&end)&~KZERO;
	if(align){
		addr += BY2PG-1;
		addr &= ~(BY2PG-1);
	}
	memset((void*)(addr|KZERO), 0, n);
	p = addr;
	addr += n;
	if(align){
		addr += BY2PG-1;
		addr &= ~(BY2PG-1);
	}
	if(addr >= (ulong)mono)
		panic("keep bill joy away");
	return (void*)(p|KZERO);
}
