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

/*
 * ATI Mach64(CT|ET|G*|V*|L*).
 */
static ushort mach64xxdid[] = {
	('C'<<8)|'T',
	('E'<<8)|'T',
	('G'<<8)|'B',
	('G'<<8)|'D',
	('G'<<8)|'I',
	('G'<<8)|'M',
	('G'<<8)|'P',
	('G'<<8)|'Q',
	('G'<<8)|'T',
	('G'<<8)|'U',
	('G'<<8)|'V',
	('G'<<8)|'Z',
	('V'<<8)|'T',
	('V'<<8)|'U',
	('V'<<8)|'V',
	('L'<<8)|'B',
	('L'<<8)|'I',
	('L'<<8)|'M',
	('L'<<8)|'P',
	0,
};

static int hwfill(VGAscr*, Rectangle, ulong);
static int hwscroll(VGAscr*, Rectangle, Rectangle);
static void initengine(VGAscr*);

static Pcidev*
mach64xxpci(void)
{
	Pcidev *p;
	ushort *did;

	if((p = pcimatch(nil, 0x1002, 0)) == nil)
		return nil;
	for(did = mach64xxdid; *did; did++){
		if(*did == p->did)
			return p;
	}

	return nil;
}

static void
mach64xxenable(VGAscr* scr)
{
	Pcidev *p;

	/*
	 * Only once, can't be disabled for now.
	 */
	if(scr->io)
		return;
	if(p = mach64xxpci()){
		scr->id = p->did;

		/*
		 * The CT doesn't always have the I/O base address
		 * in the PCI base registers. There is a way to find
		 * it via the vendor-specific PCI config space but
		 * this will do for now.
		 */
		scr->io = p->mem[1].bar & ~0x03;

		if(scr->io == 0)
			scr->io = 0x2EC;
	}
}

static ulong
mach64xxlinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, osize, oaperture;
	int i, oapsize, wasupamem;
	Pcidev *p;
	Physseg seg;

	osize = *size;
	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	if(p = mach64xxpci()){
		for(i=0; i<nelem(p->mem); i++){
			if(p->mem[i].size >= *size
			&& ((p->mem[i].bar & ~0x0F) & (*align-1)) == 0)
				break;
		}
		if(i >= nelem(p->mem)){
			print("vgamach64xx: aperture not found\n");
			return 0;
		}
		aperture = p->mem[i].bar & ~0x0F;
		*size = p->mem[i].size;
	}
	else
		aperture = 0;

	if(wasupamem)
		upafree(oaperture, oapsize);
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0))
			scr->isupamem = 1;
	}
	else
		scr->isupamem = 1;

	scr->mmio = KADDR(aperture+osize-0x400);
	if(oaperture)
		print("warning (BUG): redefinition of aperture does not change mach64mmio segment\n");
	memset(&seg, 0, sizeof(seg));
	seg.attr = SG_PHYSICAL;
	seg.name = smalloc(NAMELEN);
	snprint(seg.name, NAMELEN, "mach64mmio");
	seg.pa = aperture+osize - BY2PG;
	seg.size = BY2PG;
	addphysseg(&seg);

	seg.name = smalloc(NAMELEN);
	snprint(seg.name, NAMELEN, "mach64screen");
	seg.pa = aperture;
	seg.size = osize;
	addphysseg(&seg);

	return aperture;
}

enum {
	CrtcOffPitch	= 0x05,
	CrtcGenCtl	= 0x07,
	CurClr0		= 0x0B,		/* I/O Select */
	CurClr1		= 0x0C,
	CurOffset	= 0x0D,
	CurHVposn	= 0x0E,
	CurHVoff	= 0x0F,
	BusCntl	= 0x13,
	GenTestCntl	= 0x19,

	CrtcHsyncDis	= 0x04,
	CrtcVsyncDis	= 0x08,

	ContextMask	= 0x100,	/* not accessible via I/O */
	FifoStat,
	GuiStat,
	DpFrgdClr,
	DpBkgdClr,
	DpWriteMask,
	DpMix,
	DpPixWidth,
	DpSrc,
	ClrCmpCntl,
	GuiTrajCntl,
	ScLeftRight,
	ScTopBottom,
	DstOffPitch,
	DstYX,
	DstHeightWidth,
	DstCntl,
	DstHeight,
	DstBresErr,
	DstBresInc,
	DstBresDec,
	SrcCntl,
	SrcHeight1Width1,
	SrcHeight2Width2,
	SrcYX,
	SrcWidth1,
	SrcYXstart,
	HostCntl,
	PatReg0,
	PatReg1,
	PatCntl,
	ScBottom,
	ScLeft,
	ScRight,
	ScTop,
	ClrCmpClr,
	ClrCmpMask,
	DpChainMask,
	SrcOffPitch,	
	LcdIndex,
	LcdData,
};

enum {
	LCD_ConfigPanel = 0,
	LCD_GenCtrl,
	LCD_DstnCntl,
	LCD_HfbPitchAddr,
	LCD_HorzStretch,
	LCD_VertStretch,
	LCD_ExtVertStretch,
	LCD_LtGio,
	LCD_PowerMngmnt,
	LCD_ZvgPio,
	Nlcd,
};

static uchar mmoffset[] = {
	[CrtcOffPitch]	0x05,
	[CurClr0]	0x18,
	[CurClr1]	0x19,
	[CurOffset]	0x1A,
	[CurHVposn]	0x1B,
	[CurHVoff]	0x1C,
	[BusCntl]		0x28,
	[LcdIndex]	0x29,
	[LcdData]	0x2A,
	[GenTestCntl]	0x34,
	[DstOffPitch]	0x40,
	[DstYX]		0x43,
	[DstHeight]	0x45,
	[DstHeightWidth]	0x46,
	[DstBresErr]	0x49,
	[DstBresInc]	0x4A,
	[DstBresDec]	0x4B,
	[DstCntl]		0x4C,
	[SrcOffPitch]	0x60,
	[SrcYX]	0x63,
	[SrcWidth1]	0x64,
	[SrcYXstart]	0x69,
	[SrcHeight1Width1]	0x66,
	[SrcHeight2Width2]	0x6C,
	[SrcCntl]		0x6D,
	[HostCntl]	0x90,
	[PatReg0]	0xA0,
	[PatReg1]	0xA1,
	[PatCntl]	0xA2,
	[ScLeft]	0xA8,
	[ScRight]	0xA9,
	[ScLeftRight]	0xAA,
	[ScTop]	0xAB,
	[ScBottom] 0xAC,
	[ScTopBottom]	0xAD,
	[DpBkgdClr]	0xB0,
	[DpFrgdClr]	0xB1,
	[DpWriteMask]	0xB2,
	[DpChainMask]	0xB3,
	[DpPixWidth]	0xB4,
	[DpMix]		0xB5,
	[DpSrc]		0xB6,
	[ClrCmpClr]	0xC0,
	[ClrCmpMask]	0xC1,
	[ClrCmpCntl]	0xC2,
	[FifoStat]		0xC4,
	[ContextMask]	0xC8,
	[GuiTrajCntl]	0xCC,
	[GuiStat]		0xCE,
};

static ulong
ior32(VGAscr* scr, int r)
{
	if(scr->io == 0x2EC || scr->io == 0x1C8)
		return inl((r<<10)+scr->io);
	if(r >= 0x100 && scr->mmio != nil)
		return scr->mmio[mmoffset[r]];
	return inl((mmoffset[r]<<2)+scr->io);
}

static void
iow32(VGAscr* scr, int r, ulong l)
{
	if(scr->io == 0x2EC || scr->io == 0x1C8)
		outl(((r)<<10)+scr->io, l);
	else if(r >= 0x100 && scr->mmio != nil)
		scr->mmio[mmoffset[r]] = l;
	else
		outl((mmoffset[r]<<2)+scr->io, l);
}

static ulong
lcdr32(VGAscr *scr, ulong r)
{
	ulong or;

	or = ior32(scr, LcdIndex);
	iow32(scr, LcdIndex, (or&~0x0F) | (r&0x0F));
	return ior32(scr, LcdData);
}

static void
lcdw32(VGAscr *scr, ulong r, ulong v)
{
	ulong or;

	or = ior32(scr, LcdIndex);
	iow32(scr, LcdIndex, (or&~0x0F) | (r&0x0F));
	iow32(scr, LcdData, v);
}

static void
mach64xxcurdisable(VGAscr* scr)
{
	ulong r;

	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);
}

static void
mach64xxcurload(VGAscr* scr, Cursor* curs)
{
	uchar *p;
	int i, y;
	ulong c, s, m, r;

	/*
	 * Disable the cursor.
	 */
	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);

	p = KADDR(scr->aperture);
	p += scr->storage;

	/*
	 * Initialise the 64x64 cursor RAM array.
	 * The cursor mode gives the following truth table:
	 *	p1 p0	colour
	 *	 0  0	Cursor Colour 0
	 *	 0  1	Cursor Colour 1
	 *	 1  0	Transparent
	 *	 1  1	Complement
	 * Put the cursor into the top-right of the 64x64 array.
	 */
	for(y = 0; y < 16; y++){
		for(i = 0; i < (64-16)/8; i++){
			*p++ = 0xAA;
			*p++ = 0xAA;
		}

		c = (curs->clr[2*y]<<8)|curs->clr[y*2 + 1];
		s = (curs->set[2*y]<<8)|curs->set[y*2 + 1];

		m = 0x00000000;
		for(i = 0; i < 16; i++){
			if(s & (1<<(15-i)))
				m |= 0x01<<(2*i);
			else if(c & (1<<(15-i)))
				;
			else
				m |= 0x02<<(2*i);
		}
		*p++ = m;
		*p++ = m>>8;
		*p++ = m>>16;
		*p++ = m>>24;
	}
	memset(p, 0xAA, (64-16)*16);

	/*
	 * Set the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	iow32(scr, GenTestCntl, 0x80|r);
}

static int
ptalmostinrect(Point p, Rectangle r)
{
	return p.x>=r.min.x && p.x<=r.max.x &&
	       p.y>=r.min.y && p.y<=r.max.y;
}

/*
 * If necessary, translate the rectangle physr
 * some multiple of [dx dy] so that it includes p.
 * Return 1 if the rectangle changed.
 */
static int
screenpan(Point p, Rectangle *physr, int dx, int dy)
{
	int d;

	if(ptalmostinrect(p, *physr))
		return 0;

	if(p.y < physr->min.y){
		d = physr->min.y - (p.y&~(dy-1));
		physr->min.y -= d;
		physr->max.y -= d;
	}
	if(p.y > physr->max.y){
		d = ((p.y+dy-1)&~(dy-1)) - physr->max.y;
		physr->min.y += d;
		physr->max.y += d;
	}

	if(p.x < physr->min.x){
		d = physr->min.x - (p.x&~(dx-1));
		physr->min.x -= d;
		physr->max.x -= d;
	}
	if(p.x > physr->max.x){
		d = ((p.x+dx-1)&~(dx-1)) - physr->max.x;
		physr->min.x += d;
		physr->max.x += d;
	}
	return 1;
}

static int
mach64xxcurmove(VGAscr* scr, Point p)
{
	int x, xo, y, yo;
	int dx;
	ulong off, pitch;

	/*
	 * If the point we want to display is outside the current
	 * screen rectangle, pan the screen to display it.
	 *
	 * We have to move in 64-bit chunks.
	 */
	if(scr->gscreen->depth == 24)
		dx = (64*3)/24;
	else
		dx = 64 / scr->gscreen->depth;

	if(screenpan(p, &physgscreenr, dx, 1)){
		off = (physgscreenr.min.y*Dx(scr->gscreen->r)+physgscreenr.min.x)/dx;
		pitch = Dx(scr->gscreen->r)/8;
		iow32(scr, CrtcOffPitch, (pitch<<22)|off);
	}

	p.x -= physgscreenr.min.x;
	p.y -= physgscreenr.min.y;

	/*
	 * Mustn't position the cursor offscreen even partially,
	 * or it disappears. Therefore, if x or y is -ve, adjust the
	 * cursor presets instead. If y is negative also have to
	 * adjust the starting offset.
	 */
	if((x = p.x+scr->offset.x) < 0){
		xo = x;
		x = 0;
	}
	else
		xo = 0;
	if((y = p.y+scr->offset.y) < 0){
		yo = y;
		y = 0;
	}
	else
		yo = 0;

	iow32(scr, CurHVoff, ((64-16-yo)<<16)|(64-16-xo));
	iow32(scr, CurOffset, scr->storage/8 + (-yo*2));
	iow32(scr, CurHVposn, (y<<16)|x);

	return 0;
}

static void
mach64xxcurenable(VGAscr* scr)
{
	ulong r, storage;

	mach64xxenable(scr);
	if(scr->io == 0)
		return;

	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);

	iow32(scr, CurClr0, (Pwhite<<24)|(Pwhite<<16)|(Pwhite<<8)|Pwhite);
	iow32(scr, CurClr1, (Pblack<<24)|(Pblack<<16)|(Pblack<<8)|Pblack);

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be 64-bit aligned.
	 */
	storage = (scr->gscreen->width*BY2WD*scr->gscreen->r.max.y+7)/8;
	iow32(scr, CurOffset, storage);
	scr->storage = storage*8;

	/*
	 * Cursor goes in the top right corner of the 64x64 array
	 * so the horizontal and vertical presets are 64-16.
	 */
	iow32(scr, CurHVposn, (0<<16)|0);
	iow32(scr, CurHVoff, ((64-16)<<16)|(64-16));

	/*
	 * Load, locate and enable the 64x64 cursor.
	 */
	mach64xxcurload(scr, &arrow);
	mach64xxcurmove(scr, ZP);
	iow32(scr, GenTestCntl, 0x80|r);
}

static void
waitforfifo(VGAscr *scr, int entries)
{
	int x;
	x = 0;
	while((ior32(scr, FifoStat)&0xFF) > (0x8000>>entries) && x++ < 1000000)
		;
	if(x >= 1000000)
		iprint("fifo %d stat %.8lux %.8lux scrio %.8lux mmio %p scr %p pc %luX\n", entries, ior32(scr, FifoStat), scr->mmio[mmoffset[FifoStat]], scr->io, scr->mmio, scr, getcallerpc(&scr));
}

static void
waitforidle(VGAscr *scr)
{
	int x;
	waitforfifo(scr, 16);
	x = 0;
	while((ior32(scr, GuiStat)&1) && x++ < 1000000)
		;
	if(x >= 1000000)
		iprint("idle stat %.8lux %.8lux scrio %.8lux mmio %p scr %p pc %luX\n", ior32(scr, GuiStat), scr->mmio[mmoffset[GuiStat]], scr->io, scr->mmio, scr, getcallerpc(&scr));
}

static void
resetengine(VGAscr *scr)
{
	ulong x;
	x = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, x&~0x100);
	iow32(scr, GenTestCntl, x|0x100);
	iow32(scr, BusCntl, ior32(scr, BusCntl)|0x00A00000);
}

static void
initengine(VGAscr *scr)
{
	ulong pitch;

	pitch = Dx(scr->gscreen->r)/8;
	if(scr->gscreen->depth == 24)
		pitch *= 3;

	resetengine(scr);
	waitforfifo(scr, 14);
	iow32(scr, ContextMask, ~0);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstYX, 0);
	iow32(scr, DstHeight, 0);
	iow32(scr, DstBresErr, 0);
	iow32(scr, DstBresInc, 0);
	iow32(scr, DstBresDec, 0);
	iow32(scr, DstCntl, 0x23);
	iow32(scr, SrcOffPitch, pitch<<22);
	iow32(scr, SrcYX, 0);
	iow32(scr, SrcHeight1Width1, 1);
	iow32(scr, SrcYXstart, 0);
	iow32(scr, SrcHeight2Width2, 1);
	iow32(scr, SrcCntl, 0x01);

	waitforfifo(scr, 13);
	iow32(scr, HostCntl, 0);
	iow32(scr, PatReg0, 0);
	iow32(scr, PatReg1, 0);
	iow32(scr, PatCntl, 0);
	iow32(scr, ScLeft, 0);
	iow32(scr, ScTop, 0);
	iow32(scr, ScBottom, 0xFFFF);
	iow32(scr, ScRight, 0xFFFF);
	iow32(scr, DpBkgdClr, 0);
	iow32(scr, DpFrgdClr, ~0);
	iow32(scr, DpWriteMask, ~0);
	iow32(scr, DpMix, 0x70003);
	iow32(scr, DpSrc, 0x00010100);

	waitforfifo(scr, 3);
	iow32(scr, ClrCmpClr, 0);
	iow32(scr, ClrCmpMask, ~0);
	iow32(scr, ClrCmpCntl, 0);

	waitforfifo(scr, 2);
	switch(scr->gscreen->depth){
	case 8:
	case 24:	/* [sic] */
		iow32(scr, DpPixWidth, 0x00020202);
		iow32(scr, DpChainMask, 0x8080);
		break;
	case 16:
		iow32(scr, DpPixWidth, 0x00040404);
		iow32(scr, DpChainMask, 0x8410);
		break;
	case 32:
		iow32(scr, DpPixWidth, 0x00060606);
		iow32(scr, DpChainMask, 0x8080);
		break;
	}

	waitforidle(scr);
}

static int
mach64hwfill(VGAscr *scr, Rectangle r, ulong sval)
{
	ulong pitch;
	ulong ctl;

if(drawdebug)
	iprint("hwfill %R val %lux...\n", r, sval);

	/* shouldn't happen */
	if(scr->io == 0x2EC || scr->io == 0x1C8 || scr->io == 0)
		return 0;

	pitch = Dx(scr->gscreen->r)/8;
	ctl = 1|2;	/* left-to-right, top-to-bottom */
	if(scr->gscreen->depth == 24){
		r.min.x *= 3;
		r.max.x *= 3;
		pitch *= 3;
		ctl |= (1<<7)|(((r.min.x/4)%6)<<8);
	}

	waitforfifo(scr, 11);
	iow32(scr, DpFrgdClr, sval);
	iow32(scr, DpWriteMask, 0xFFFFFFFF);
	iow32(scr, DpMix, 0x00070003);
	iow32(scr, DpSrc, 0x00000111);
	iow32(scr, ClrCmpCntl, 0x00000000);
	iow32(scr, ScLeftRight, 0x1FFF0000);
	iow32(scr, ScTopBottom, 0x1FFF0000);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstCntl, ctl);
	iow32(scr, DstYX, (r.min.x<<16)|r.min.y);
	iow32(scr, DstHeightWidth, (Dx(r)<<16)|Dy(r));

	waitforidle(scr);
	return 1;
}

static int
mach64hwscroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	ulong pitch;
	Point dp, sp;
	ulong ctl;
	int dx, dy;

	dx = Dx(r);
	dy = Dy(r);
	pitch = Dx(scr->gscreen->r)/8;
	if(scr->gscreen->depth == 24){
		dx *= 3;
		pitch *= 3;
		r.min.x *= 3;
		sr.min.x *= 3;
	}

	ctl = 0;
	if(r.min.x <= sr.min.x){
		ctl |= 1;
		dp.x = r.min.x;
		sp.x = sr.min.x;
	}else{
		dp.x = r.min.x+dx-1;
		sp.x = sr.min.x+dx-1;
	}

	if(r.min.y <= sr.min.y){
		ctl |= 2;
		dp.y = r.min.y;
		sp.y = sr.min.y;
	}else{
		dp.y = r.min.y+dy-1;
		sp.y = sr.min.y+dy-1;
	}

	if(scr->gscreen->depth == 24)
		ctl |= (1<<7)|(((dp.x/4)%6)<<8);

	waitforfifo(scr, 6);
	iow32(scr, ScLeftRight, 0x1FFF0000);
	iow32(scr, ScTopBottom, 0x1FFF0000);
	iow32(scr, DpWriteMask, 0xFFFFFFFF);
	iow32(scr, DpMix, 0x00070003);
	iow32(scr, DpSrc, 0x00000300);
	iow32(scr, ClrCmpCntl, 0x00000000);

	waitforfifo(scr, 8);
	iow32(scr, SrcOffPitch, pitch<<22);
	iow32(scr, SrcCntl, 0x00000000);
	iow32(scr, SrcYX, (sp.x<<16)|sp.y);
	iow32(scr, SrcWidth1, dx);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstCntl, ctl);

	iow32(scr, DstYX, (dp.x<<16)|dp.y);
	iow32(scr, DstHeightWidth, (dx<<16)|dy);

	waitforidle(scr);

	return 1;
}

/*
 * This should work, but doesn't.
 * It messes up the screen timings for some reason.
 */
static void
mach64blank(VGAscr *scr, int blank)
{
	ulong ctl;

	ctl = ior32(scr, CrtcGenCtl) & ~(CrtcHsyncDis|CrtcVsyncDis);
	if(blank)
		ctl |= CrtcHsyncDis|CrtcVsyncDis;
	iow32(scr, CrtcGenCtl, ctl);
}

/*
 * We squirrel away whether the LCD and/or CRT were
 * on when we were called to blank the screen, and
 * restore the old state.  If we are called to blank the
 * screen when it is already blank, we don't update the state.
 * Such a call sequence should not happen, though.
 *
 * We could try forcing the chip into power management
 * mode instead, but I'm not sure how that would interact
 * with screen updates going on while the screen is blanked.
 */
static void
mach64lcdblank(VGAscr *scr, int blank)
{
	static int crtlcd;
	ulong x;

	if(blank) {
		x = lcdr32(scr, LCD_GenCtrl);
		if(x & 3) {
			crtlcd = x & 3;
			lcdw32(scr, LCD_GenCtrl,  x&~3);
		}
	} else {
		if(crtlcd == 0)
			crtlcd = 2;	/* lcd only */
		x = lcdr32(scr, LCD_GenCtrl);
		lcdw32(scr, LCD_GenCtrl, x | crtlcd);
	}
}

static void
mach64xxdrawinit(VGAscr *scr)
{
	if(scr->io > 0x2FF){
		initengine(scr);
		scr->fill = mach64hwfill;
		scr->scroll = mach64hwscroll;
	}
/*	scr->blank = mach64blank; */
	switch(scr->id){
	default:
		break;
	case ('L'<<8)|'B':		/* 4C42: Rage 3D LTPro */
	case ('L'<<8)|'I':		/* 4C49: Rage 3D LTPro */
	case ('L'<<8)|'M':		/* 4C4D: Rage Mobility */
	case ('L'<<8)|'P':		/* 4C50: Rage 3D LTPro */
		scr->blank = mach64lcdblank;
		break;
	}
}

VGAdev vgamach64xxdev = {
	"mach64xx",

	mach64xxenable,			/* enable */
	0,				/* disable */
	0,				/* page */
	mach64xxlinear,			/* linear */
	mach64xxdrawinit,	/* drawinit */
};

VGAcur vgamach64xxcur = {
	"mach64xxhwgc",

	mach64xxcurenable,		/* enable */
	mach64xxcurdisable,		/* disable */
	mach64xxcurload,		/* load */
	mach64xxcurmove,		/* move */

	1					/* doespanning */
};

