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

enum {
	Pramin = 0x00710000,
	Pramdac = 0x00680000,
	Fifo = 0x00800000,
	Pgraph = 0x00400000
};

enum {
	hwCurPos = Pramdac + 0x0300,
	hwCurImage = Pramin + (0x00010000 - 0x0800),
};

/* Nvidia is good about backwards compatibility -- any did >= 0x20 is fine */
static Pcidev*
nvidiapci(void)
{
	Pcidev *p;

	p = nil;
	while((p = pcimatch(p, 0x10DE, 0)) != nil){
		if(p->did >= 0x20 && p->ccrb == 3)	/* video card */
			return p;
	}
	return nil;
}

static ulong
nvidialinear(VGAscr* scr, int* size, int* align)
{
	Pcidev *p;
	int oapsize, wasupamem;
	ulong aperture, oaperture;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	aperture = 0;
	if(p = nvidiapci()){
		aperture = p->mem[1].bar & ~0x0F;
		*size = p->mem[1].size;
	}

	if(wasupamem){
		if(oaperture == aperture)
			return oaperture;
		upafree(oaperture, oapsize);
	}
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0)){
			aperture = oaperture;
			scr->isupamem = 1;
		}
		else
			scr->isupamem = 0;
	}
	else
		scr->isupamem = 1;

	return aperture;
}

static void
nvidiaenable(VGAscr* scr)
{
	Pcidev *p;
	ulong aperture;
	int align, size;

	/*
	 * Only once, can't be disabled for now.
	 * scr->io holds the physical address of
	 * the MMIO registers.
	 */
	if(scr->io)
		return;
	p = nvidiapci();
	if(p == nil)
		return;
	scr->id = p->did;

	scr->io = upamalloc(p->mem[0].bar & ~0x0F, p->mem[0].size, 0);
	if(scr->io == 0)
		return;
	addvgaseg("nvidiammio", scr->io, p->mem[0].size);

	size = p->mem[1].size;
	align = 0;
	aperture = nvidialinear(scr, &size, &align);
	if(aperture){
		scr->aperture = aperture;
		scr->apsize = size;
		addvgaseg("nvidiascreen", aperture, size);
	}
}

static void
nvidiacurdisable(VGAscr* scr)
{
	if(scr->io == 0)
		return;

	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) & ~0x01);
}

static void
nvidiacurload(VGAscr* scr, Cursor* curs)
{
	ulong*	p;
	int	i,j;
	ushort	c,s;
	ulong	tmp;

	if(scr->io == 0)
		return;

	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) & ~0x01);

	p = KADDR(scr->io + hwCurImage);

	for(i=0; i<16; i++) {
		switch(scr->id){
		default:
			c = (curs->clr[2 * i] << 8) | curs->clr[2 * i+1];
			s = (curs->set[2 * i] << 8) | curs->set[2 * i+1];
			break;
		case 0x171:		/* for Geforece4 MX bug, K.Okamoto */
		case 0x181:
			c = (curs->clr[2 * i+1] << 8) | curs->clr[2 * i];
			s = (curs->set[2 * i+1] << 8) | curs->set[2 * i];
			break;
		}
		tmp = 0;
		for (j=0; j<16; j++){
			if(s&0x8000)
				tmp |= 0x80000000;
			else if(c&0x8000)
				tmp |= 0xFFFF0000;
			if (j&0x1){
				*p++ = tmp;
				tmp = 0;
			} else {
				tmp>>=16;
			}
			c<<=1;
			s<<=1;
		}
		for (j=0; j<8; j++)
			*p++ = 0;
	}
	for (i=0; i<256; i++)
		*p++ = 0;

	scr->offset = curs->offset;
	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) | 0x01);

	return;
}

static int
nvidiacurmove(VGAscr* scr, Point p)
{
	ulong*	cursorpos;

	if(scr->io == 0)
		return 1;

	cursorpos = KADDR(scr->io + hwCurPos);
	*cursorpos = ((p.y+scr->offset.y)<<16)|((p.x+scr->offset.x) & 0xFFFF);

	return 0;
}

static void
nvidiacurenable(VGAscr* scr)
{
	nvidiaenable(scr);
	if(scr->io == 0)
		return;

	vgaxo(Crtx, 0x1F, 0x57);

	nvidiacurload(scr, &arrow);
	nvidiacurmove(scr, ZP);

	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) | 0x01);
}

enum {
	RopFifo 		= 0x00000000, 
	ClipFifo 		= 0x00002000,
	PattFifo 		= 0x00004000,
	BltFifo 		= 0x00008000,
	BitmapFifo 	= 0x0000A000,
};

enum {
	RopRop3 = RopFifo + 0x300,

	ClipTopLeft = ClipFifo + 0x300,
	ClipWidthHeight = ClipFifo + 0x304,

	PattShape = PattFifo + 0x0308,
	PattColor0 = PattFifo + 0x0310,
	PattColor1 = PattFifo + 0x0314,
	PattMonochrome0 = PattFifo + 0x0318,
	PattMonochrome1 = PattFifo + 0x031C,

	BltTopLeftSrc = BltFifo + 0x0300,
	BltTopLeftDst = BltFifo + 0x0304,
	BltWidthHeight = BltFifo + 0x0308,

	BitmapColor1A = BitmapFifo + 0x03FC,
	BitmapURect0TopLeft = BitmapFifo + 0x0400,
	BitmapURect0WidthHeight = BitmapFifo + 0x0404,
};

static void
waitforidle(VGAscr *scr)
{
	ulong*	pgraph;
	int x;

	pgraph = KADDR(scr->io + Pgraph);

	x = 0;
	while(pgraph[0x00000700/4] & 0x01 && x++ < 1000000)
		;

	if(x >= 1000000)
		iprint("idle stat %lud scrio %.8lux scr %p pc %luX\n", *pgraph, scr->io, scr, getcallerpc(&scr));
}

static void
waitforfifo(VGAscr *scr, int fifo, int entries)
{
	ushort* fifofree;
	int x;

	x = 0;
	fifofree = KADDR(scr->io + Fifo + fifo + 0x10);

	while(((*fifofree >> 2) < entries) && x++ < 1000000)
		;

	if(x >= 1000000)
		iprint("fifo stat %d scrio %.8lux scr %p pc %luX\n", *fifofree, scr->io, scr, getcallerpc(&scr));
}

static int
nvidiahwfill(VGAscr *scr, Rectangle r, ulong sval)
{
	ulong*	fifo;

	fifo = KADDR(scr->io + Fifo);

	waitforfifo(scr, BitmapFifo, 1);

	fifo[BitmapColor1A/4] = sval;

	waitforfifo(scr, BitmapFifo, 2);

	fifo[BitmapURect0TopLeft/4] = (r.min.x << 16) | r.min.y;
	fifo[BitmapURect0WidthHeight/4] = (Dx(r) << 16) | Dy(r);

	waitforidle(scr);

	return 1;
}

static int
nvidiahwscroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	ulong*	fifo;

	fifo = KADDR(scr->io + Fifo);

	waitforfifo(scr, BltFifo, 3);

	fifo[BltTopLeftSrc/4] = (sr.min.y << 16) | sr.min.x;
	fifo[BltTopLeftDst/4] = (r.min.y << 16) | r.min.x;
	fifo[BltWidthHeight/4] = (Dy(r) << 16) | Dx(r);

	waitforidle(scr);

	return 1;
}

void
nvidiablank(VGAscr*, int blank)
{
	uchar seq1, crtc1A;

	seq1 = vgaxi(Seqx, 1) & ~0x20;
	crtc1A = vgaxi(Crtx, 0x1A) & ~0xC0;

	if(blank){
		seq1 |= 0x20;
//		crtc1A |= 0xC0;
		crtc1A |= 0x80;
	}

	vgaxo(Seqx, 1, seq1);
	vgaxo(Crtx, 0x1A, crtc1A);
}

static void
nvidiadrawinit(VGAscr *scr)
{
	ulong*	fifo;

	fifo = KADDR(scr->io + Fifo);

	waitforfifo(scr, ClipFifo, 2);

	fifo[ClipTopLeft/4] = 0x0;
	fifo[ClipWidthHeight/4] = 0x80008000;

	waitforfifo(scr, PattFifo, 5);

	fifo[PattShape/4] = 0;
	fifo[PattColor0/4] = 0xffffffff;
	fifo[PattColor1/4] = 0xffffffff;
	fifo[PattMonochrome0/4] = 0xffffffff;
	fifo[PattMonochrome1/4] = 0xffffffff;

	waitforfifo(scr, RopFifo, 1);

	fifo[RopRop3/4] = 0xCC;

	waitforidle(scr);

	scr->blank = nvidiablank;
	hwblank = 1;
	scr->fill = nvidiahwfill;
	scr->scroll = nvidiahwscroll;
}

VGAdev vganvidiadev = {
	"nvidia",

	nvidiaenable,
	nil,
	nil,
	nvidialinear,
	nvidiadrawinit,
};

VGAcur vganvidiacur = {
	"nvidiahwgc",

	nvidiacurenable,
	nvidiacurdisable,
	nvidiacurload,
	nvidiacurmove,
};
