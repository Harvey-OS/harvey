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

static int
s3pageset(VGAscr* scr, int page)
{
	uchar crt35, crt51;
	int opage;

	crt35 = vgaxi(Crtx, 0x35);
	if(scr->gscreen->depth >= 8){
		/*
		 * The S3 registers need to be unlocked for this.
		 * Let's hope they are already:
		 *	vgaxo(Crtx, 0x38, 0x48);
		 *	vgaxo(Crtx, 0x39, 0xA0);
		 *
		 * The page is 6 bits, the lower 4 bits in Crt35<3:0>,
		 * the upper 2 in Crt51<3:2>.
		 */
		vgaxo(Crtx, 0x35, page & 0x0F);
		crt51 = vgaxi(Crtx, 0x51);
		vgaxo(Crtx, 0x51, (crt51 & ~0x0C)|((page & 0x30)>>2));
		opage = ((crt51 & 0x0C)<<2)|(crt35 & 0x0F);
	}
	else{
		vgaxo(Crtx, 0x35, (page<<2) & 0x0C);
		opage = (crt35>>2) & 0x03;
	}

	return opage;
}

static void
s3page(VGAscr* scr, int page)
{
	int id;

	id = (vgaxi(Crtx, 0x30)<<8)|vgaxi(Crtx, 0x2E);
	switch(id){

	case 0xE110:				/* ViRGE/GX2 */
		break;

	default:
		lock(&scr->devlock);
		s3pageset(scr, page);
		unlock(&scr->devlock);
		break;
	}
}

static ulong
s3linear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, oaperture, opciaddr;
	int oapsize, wasupamem;
	Pcidev *p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	opciaddr = scr->pciaddr;
	wasupamem = scr->isupamem;
	if(wasupamem)
		upafree(oaperture, oapsize);
	scr->isupamem = 0;
	scr->pciaddr = 0;

	if(p = pcimatch(nil, 0x5333, 0)){
		aperture = p->mem[0].bar & ~0x0F;
		*size = p->mem[0].size;
	}
	else
		aperture = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0)){
			scr->isupamem = 1;
			scr->pciaddr = opciaddr;
		}
	}
	else{
		scr->pciaddr = p->mem[0].bar & ~0x0F;
		scr->isupamem = 1;
	}

	return aperture;
}

static void
s3vsyncactive(void)
{
	/*
	 * Hardware cursor information is fetched from display memory
	 * during the horizontal blank active time. The 80x chips may hang
	 * if the cursor is turned on or off during this period.
	 */
	while((vgai(Status1) & 0x08) == 0)
		;
}

static void
s3disable(VGAscr*)
{
	uchar crt45;

	/*
	 * Turn cursor off.
	 */
	crt45 = vgaxi(Crtx, 0x45) & 0xFE;
	s3vsyncactive();
	vgaxo(Crtx, 0x45, crt45);
}

static void
s3enable(VGAscr* scr)
{
	int i;
	ulong storage;

	s3disable(scr);

	/*
	 * Cursor colours. Set both the CR0[EF] and the colour
	 * stack in case we are using a 16-bit RAMDAC.
	 */
	vgaxo(Crtx, 0x0E, Pwhite);
	vgaxo(Crtx, 0x0F, Pblack);
	vgaxi(Crtx, 0x45);

	for(i = 0; i < 3; i++)
		vgaxo(Crtx, 0x4A, Pblack);
	vgaxi(Crtx, 0x45);
	for(i = 0; i < 3; i++)
		vgaxo(Crtx, 0x4B, Pwhite);

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be on a 1024-byte boundary.
	 */
	storage = (scr->gscreen->width*BY2WD*scr->gscreen->r.max.y+1023)/1024;
	vgaxo(Crtx, 0x4C, (storage>>8) & 0x0F);
	vgaxo(Crtx, 0x4D, storage & 0xFF);
	storage *= 1024;
	scr->storage = storage;

	/*
	 * Enable the cursor in Microsoft Windows format.
	 */
	vgaxo(Crtx, 0x55, vgaxi(Crtx, 0x55) & ~0x10);
	s3vsyncactive();
	vgaxo(Crtx, 0x45, 0x01);
}

static void
s3load(VGAscr* scr, Cursor* curs)
{
	uchar *p;
	int id, opage, x, y;

	/*
	 * Disable the cursor and
	 * set the pointer to the two planes.
	 */
	s3disable(scr);

	opage = 0;
	p = KADDR(scr->aperture);
	id = (vgaxi(Crtx, 0x30)<<8)|vgaxi(Crtx, 0x2E);
	switch(id){

	case 0xE131:				/* ViRGE */
	case 0xE18A:				/* ViRGE/[DG]X */
	case 0xE110:				/* ViRGE/GX2 */
	case 0xE13D:				/* ViRGE/VX */
		p += scr->storage;
		break;

	default:
		lock(&scr->devlock);
		opage = s3pageset(scr, scr->storage>>16);
		p += (scr->storage & 0xFFFF);
		break;
	}

	/*
	 * The cursor is set in Microsoft Windows format (the ViRGE/GX2 no
	 * longer supports the X11 format) which gives the following truth table:
	 *	and xor	colour
	 *	 0   0	background colour
	 *	 0   1	foreground colour
	 *	 1   0	current screen pixel
	 *	 1   1	NOT current screen pixel
	 * Put the cursor into the top-left of the 64x64 array.
	 *
	 * The cursor pattern in memory is interleaved words of
	 * AND and XOR patterns.
	 */
	for(y = 0; y < 64; y++){
		for(x = 0; x < 64/8; x += 2){
			if(x < 16/8 && y < 16){
				*p++ = ~(curs->clr[2*y + x]|curs->set[2*y + x]);
				*p++ = ~(curs->clr[2*y + x+1]|curs->set[2*y + x+1]);
				*p++ = curs->set[2*y + x];
				*p++ = curs->set[2*y + x+1];
			}
			else {
				*p++ = 0xFF;
				*p++ = 0xFF;
				*p++ = 0x00;
				*p++ = 0x00;
			}
		}
	}

	switch(id){

	case 0xE131:				/* ViRGE */
	case 0xE18A:				/* ViRGE/[DG]X */
	case 0xE110:				/* ViRGE/GX2 */
	case 0xE13D:				/* ViRGE/VX */
		break;

	default:
		s3pageset(scr, opage);
		unlock(&scr->devlock);
		break;
	}

	/*
	 * Save the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	s3vsyncactive();
	vgaxo(Crtx, 0x45, 0x01);
}

static int
s3move(VGAscr* scr, Point p)
{
	int x, xo, y, yo;

	/*
	 * Mustn't position the cursor offscreen even partially,
	 * or it disappears. Therefore, if x or y is -ve, adjust the
	 * cursor offset instead.
	 * There seems to be a bug in that if the offset is 1, the
	 * cursor doesn't disappear off the left edge properly, so
	 * round it up to be even.
	 */
	if((x = p.x+scr->offset.x) < 0){
		xo = -x;
		xo = ((xo+1)/2)*2;
		x = 0;
	}
	else
		xo = 0;
	if((y = p.y+scr->offset.y) < 0){
		yo = -y;
		y = 0;
	}
	else
		yo = 0;

	vgaxo(Crtx, 0x46, (x>>8) & 0x07);
	vgaxo(Crtx, 0x47, x & 0xFF);
	vgaxo(Crtx, 0x49, y & 0xFF);
	vgaxo(Crtx, 0x4E, xo);
	vgaxo(Crtx, 0x4F, yo);
	vgaxo(Crtx, 0x48, (y>>8) & 0x07);

	return 0;
}

/*
 * The manual gives byte offsets, but we want ulong offsets, hence /4.
 */
enum {
	SrcBase = 0xA4D4/4,
	DstBase = 0xA4D8/4,
	Stride = 0xA4E4/4,
	FgrdData = 0xA4F4/4,
	WidthHeight = 0xA504/4,
	SrcXY = 0xA508/4,
	DestXY = 0xA50C/4,
	Command = 0xA500/4,
	SubStat = 0x8504/4,
	FifoStat = 0x850C/4,
};

/*
 * Wait for writes to VGA memory via linear aperture to flush.
 */
enum {Maxloop = 1<<24};
struct {
	ulong linear;
	ulong fifo;
	ulong idle;
} waitcount;

static void
waitforlinearfifo(VGAscr *scr)
{
	ulong *mmio;
	long x;
	static ulong nwaitforlinearfifo;
	ulong mask, val;

	switch(scr->id){
	default:
		panic("unknown scr->id in s3 waitforlinearfifo");
	case 0xE131:	/* ViRGE */
	case 0xE13D:	/* ViRGE/VX */
		mask = 0x0F<<6;
		val = 0x08<<6;
		break;
	case 0xE110:	/* ViRGE/GX2 */
		mask = 0x1F<<6;
		val = 0x10<<6;
		break;
	}
	mmio = scr->mmio;
	x = 0;
	while((mmio[FifoStat]&mask) != val && x++ < Maxloop)
		waitcount.linear++;
}

static void
waitforfifo(VGAscr *scr, int entries)
{
	ulong *mmio;
	long x;
	static ulong nwaitforfifo;

	mmio = scr->mmio;
	x = 0;
	while((mmio[SubStat]&0x1F00) < ((entries+2)<<8) && x++ < Maxloop)
		waitcount.fifo++;
}

static void
waitforidle(VGAscr *scr)
{
	ulong *mmio;
	long x;

	mmio = scr->mmio;
	x = 0;
	while((mmio[SubStat]&0x3F00) != 0x3000 && x++ < Maxloop)
		waitcount.idle++;
}

static int
hwscroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	enum { Bitbltop = 0xCC };	/* copy source */
	ulong *mmio;
	ulong cmd, stride;
	Point dp, sp;
	int did, d;

	d = scr->gscreen->depth;
	did = (d-8)/8;
	cmd = 0x00000020|(Bitbltop<<17)|(did<<2);
	stride = Dx(scr->gscreen->r)*d/8;

	if(r.min.x <= sr.min.x){
		cmd |= 1<<25;
		dp.x = r.min.x;
		sp.x = sr.min.x;
	}else{
		dp.x = r.max.x-1;
		sp.x = sr.max.x-1;
	}

	if(r.min.y <= sr.min.y){
		cmd |= 1<<26;
		dp.y = r.min.y;
		sp.y = sr.min.y;
	}else{
		dp.y = r.max.y-1;
		sp.y = sr.max.y-1;
	}

	mmio = scr->mmio;
	waitforlinearfifo(scr);
	waitforfifo(scr, 7);
	mmio[SrcBase] = scr->aperture;
	mmio[DstBase] = scr->aperture;
	mmio[Stride] = (stride<<16)|stride;
	mmio[WidthHeight] = ((Dx(r)-1)<<16)|Dy(r);
	mmio[SrcXY] = (sp.x<<16)|sp.y;
	mmio[DestXY] = (dp.x<<16)|dp.y;
	mmio[Command] = cmd;
	waitforidle(scr);
	return 1;
}

static int
hwfill(VGAscr *scr, Rectangle r, ulong sval)
{
	enum { Bitbltop = 0xCC };	/* copy source */
	ulong *mmio;
	ulong cmd, stride;
	int did, d;

	d = scr->gscreen->depth;
	did = (d-8)/8;
	cmd = 0x16000120|(Bitbltop<<17)|(did<<2);
	stride = Dx(scr->gscreen->r)*d/8;
	mmio = scr->mmio;
	waitforlinearfifo(scr);
	waitforfifo(scr, 8);
	mmio[SrcBase] = scr->aperture;
	mmio[DstBase] = scr->aperture;
	mmio[DstBase] = scr->aperture;
	mmio[Stride] = (stride<<16)|stride;
	mmio[FgrdData] = sval;
	mmio[WidthHeight] = ((Dx(r)-1)<<16)|Dy(r);
	mmio[DestXY] = (r.min.x<<16)|r.min.y;
	mmio[Command] = cmd;
	waitforidle(scr);
	return 1;
}

enum {
	CursorSyncCtl = 0x0D,	/* in Seqx */
	VsyncHi = 0x80,
	VsyncLo = 0x40,
	HsyncHi = 0x20,
	HsyncLo = 0x10,
};

static void
s3blank(int blank)
{
	uchar x;

	x = vgaxi(Seqx, CursorSyncCtl);
	x &= ~0xF0;
	if(blank)
		x |= VsyncLo | HsyncLo;
	vgaxo(Seqx, CursorSyncCtl, x);
}

static void
s3drawinit(VGAscr *scr)
{
	ulong id;

	id = (vgaxi(Crtx, 0x30)<<8)|vgaxi(Crtx, 0x2E);
	scr->id = id;

	/*
	 * It's highly likely that other ViRGEs will work without
	 * change to the driver, with the exception of the size of
	 * the linear aperture memory write FIFO.  Since we don't
	 * know that size, I'm not turning them on.  See waitforlinearfifo
	 * above.
	 */
	switch(id){
	case 0xE131:				/* ViRGE */
	case 0xE13D:				/* ViRGE/VX */
	case 0xE110:				/* ViRGE/GX2 */
		scr->mmio = (ulong*)(scr->pciaddr+0x1000000);
/*
 * Untested on the Alpha.
 */
/*
		scr->fill = hwfill;
		scr->scroll = hwscroll;
*/
		/* scr->blank = hwblank; */
	}
}

VGAdev vgas3dev = {
	"s3",

	0,
	0,
	s3page,
	s3linear,
	s3drawinit,
};

VGAcur vgas3cur = {
	"s3hwgc",

	s3enable,
	s3disable,
	s3load,
	s3move,
};
