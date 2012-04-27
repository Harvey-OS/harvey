#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

static Memimage* back;
static Memimage *conscol;

static Point curpos;
static Rectangle window;
static int *xp;
static int xbuf[256];
Lock vgascreenlock;
int drawdebug;

void
vgaimageinit(ulong chan)
{
	if(back == nil){
		back = allocmemimage(Rect(0,0,1,1), chan);	/* RSC BUG */
		if(back == nil)
			panic("back alloc");		/* RSC BUG */
		back->flags |= Frepl;
		back->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
		memfillcolor(back, DBlack);
	}

	if(conscol == nil){
		conscol = allocmemimage(Rect(0,0,1,1), chan);	/* RSC BUG */
		if(conscol == nil)
			panic("conscol alloc");	/* RSC BUG */
		conscol->flags |= Frepl;
		conscol->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
		memfillcolor(conscol, DWhite);
	}
}

static void
vgascroll(VGAscr* scr)
{
	int h, o;
	Point p;
	Rectangle r;

	h = scr->memdefont->height;
	o = 8*h;
	r = Rpt(window.min, Pt(window.max.x, window.max.y-o));
	p = Pt(window.min.x, window.min.y+o);
	memimagedraw(scr->gscreen, r, scr->gscreen, p, nil, p, S);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(scr->gscreen, r, back, ZP, nil, ZP, S);

	curpos.y -= o;
}

static void
vgascreenputc(VGAscr* scr, char* buf, Rectangle *flushr)
{
	Point p;
	int h, w, pos;
	Rectangle r;

//	drawdebug = 1;
	if(xp < xbuf || xp >= &xbuf[sizeof(xbuf)])
		xp = xbuf;

	h = scr->memdefont->height;
	switch(buf[0]){

	case '\n':
		if(curpos.y+h >= window.max.y){
			vgascroll(scr);
			*flushr = window;
		}
		curpos.y += h;
		vgascreenputc(scr, "\r", flushr);
		break;

	case '\r':
		xp = xbuf;
		curpos.x = window.min.x;
		break;

	case '\t':
		p = memsubfontwidth(scr->memdefont, " ");
		w = p.x;
		if(curpos.x >= window.max.x-4*w)
			vgascreenputc(scr, "\n", flushr);

		pos = (curpos.x-window.min.x)/w;
		pos = 4-(pos%4);
		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+pos*w, curpos.y + h);
		memimagedraw(scr->gscreen, r, back, back->r.min, nil, back->r.min, S);
		curpos.x += pos*w;
		break;

	case '\b':
		if(xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y+h);
		memimagedraw(scr->gscreen, r, back, back->r.min, nil, ZP, S);
		combinerect(flushr, r);
		curpos.x = *xp;
		break;

	case '\0':
		break;

	default:
		p = memsubfontwidth(scr->memdefont, buf);
		w = p.x;

		if(curpos.x >= window.max.x-w)
			vgascreenputc(scr, "\n", flushr);

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x+w, curpos.y+h);
		memimagedraw(scr->gscreen, r, back, back->r.min, nil, back->r.min, S);
		memimagestring(scr->gscreen, curpos, conscol, ZP, scr->memdefont, buf);
		combinerect(flushr, r);
		curpos.x += w;
	}
//	drawdebug = 0;
}

static void
vgascreenputs(char* s, int n)
{
	int i, gotdraw;
	Rune r;
	char buf[4];
	VGAscr *scr;
	Rectangle flushr;

	scr = &vgascreen[0];

	if(!islo()){
		/*
		 * Don't deadlock trying to
		 * print in an interrupt.
		 */
		if(!canlock(&vgascreenlock))
			return;
	}
	else
		lock(&vgascreenlock);

	/*
	 * Be nice to hold this, but not going to deadlock
	 * waiting for it.  Just try and see.
	 */
	gotdraw = canqlock(&drawlock);

	flushr = Rect(10000, 10000, -10000, -10000);

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
		vgascreenputc(scr, buf, &flushr);
	}
	flushmemscreen(flushr);

	if(gotdraw)
		qunlock(&drawlock);
	unlock(&vgascreenlock);
}

void
vgascreenwin(VGAscr* scr)
{
	int h, w;

	h = scr->memdefont->height;
	w = scr->memdefont->info[' '].width;

	window = insetrect(scr->gscreen->r, 48);
	window.max.x = window.min.x+((window.max.x-window.min.x)/w)*w;
	window.max.y = window.min.y+((window.max.y-window.min.y)/h)*h;
	curpos = window.min;

	screenputs = vgascreenputs;
}

/*
 * Supposedly this is the way to turn DPMS
 * monitors off using just the VGA registers.
 * Unfortunately, it seems to mess up the video mode
 * on the cards I've tried.
 */
void
vgablank(VGAscr*, int blank)
{
	uchar seq1, crtc17;

	if(blank) {
		seq1 = 0x00;
		crtc17 = 0x80;
	} else {
		seq1 = 0x20;
		crtc17 = 0x00;
	}

	outs(Seqx, 0x0100);			/* synchronous reset */
	seq1 |= vgaxi(Seqx, 1) & ~0x20;
	vgaxo(Seqx, 1, seq1);
	crtc17 |= vgaxi(Crtx, 0x17) & ~0x80;
	delay(10);
	vgaxo(Crtx, 0x17, crtc17);
	outs(Crtx, 0x0300);				/* end synchronous reset */
}

void
addvgaseg(char *name, ulong pa, ulong size)
{
	Physseg seg;

	memset(&seg, 0, sizeof seg);
	seg.attr = SG_PHYSICAL;
	seg.name = name;
	seg.pa = pa;
	seg.size = size;
	addphysseg(&seg);
}

void
cornerstring(char *s)
{
	int h, w;
	VGAscr *scr;
	Rectangle r;
	Point p;

	scr = &vgascreen[0];
	if(scr->vaddr == nil || screenputs != vgascreenputs)
		return;
	p = memsubfontwidth(scr->memdefont, s);
	w = p.x;
	h = scr->memdefont->height;

	r = Rect(0, 0, w, h);
	memimagedraw(scr->gscreen, r, back, back->r.min, nil, back->r.min, S);
	memimagestring(scr->gscreen, r.min, conscol, ZP, scr->memdefont, s);
//	flushmemscreen(r);
}
