
/* Portions of this file derived from work with the following copyright */

 /***************************************************************************\
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/

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
#include "nv_dma.h"

enum {
	Pramin = 0x00710000,
	Pramdac = 0x00680000,
	Fifo = 0x00800000,
	Pgraph = 0x00400000,
	Pfb = 0x00100000
};

enum {
	hwCurPos = Pramdac + 0x0300,
};

#define SKIPS 8

struct {
	ulong	*dmabase;
	int		dmacurrent;
	int		dmaput;
	int		dmafree;
	int		dmamax;
} nv;

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

static void
nvidialinear(VGAscr*, int, int)
{
}

static void
nvidiaenable(VGAscr* scr)
{
	Pcidev *p;
	ulong *q;
	int tmp;

	if(scr->mmio)
		return;
	p = nvidiapci();
	if(p == nil)
		return;
	scr->id = p->did;
	scr->pci = p;

	scr->mmio = vmap(p->mem[0].bar & ~0x0F, p->mem[0].size);
	if(scr->mmio == nil)
		return;
	addvgaseg("nvidiammio", p->mem[0].bar&~0x0F, p->mem[0].size);

	vgalinearpci(scr);
	if(scr->apsize)
		addvgaseg("nvidiascreen", scr->paddr, scr->apsize);

	/* find video memory size */
	switch (scr->id & 0x0ff0) {
	case 0x0020:
	case 0x00A0:
		q = (void*)((uchar*)scr->mmio + Pfb);
		tmp = *q;
		if (tmp & 0x0100) {
			scr->storage = ((tmp >> 12) & 0x0F) * 1024 + 1024 * 2;
		} else {
			tmp &= 0x03;
			if (tmp)
				scr->storage = (1024*1024*2) << tmp;
			else
				scr->storage = 1024*1024*32;
		}
		break;
	case 0x01A0:
		p = pcimatchtbdf(MKBUS(BusPCI, 0, 0, 1));
		tmp = pcicfgr32(p, 0x7C);
		scr->storage = (((tmp >> 6) & 31) + 1) * 1024 * 1024;
		break;
	case 0x01F0:
		p = pcimatchtbdf(MKBUS(BusPCI, 0, 0, 1));
		tmp = pcicfgr32(p, 0x84);
		scr->storage = (((tmp >> 4) & 127) + 1) * 1024 * 1024;
		break;
	default:
		q = (void*)((uchar*)scr->mmio + Pfb + 0x020C);
		tmp = (*q >> 20) & 0xFFF;
		if (tmp == 0)
			tmp = 16;
		scr->storage = tmp*1024*1024;
		break;
	}
}

static void
nvidiacurdisable(VGAscr* scr)
{
	if(scr->mmio == 0)
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

	if(scr->mmio == 0)
		return;

	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) & ~0x01);

	switch (scr->id & 0x0ff0) {
	case 0x0020:
	case 0x00A0:
		p = (void*)((uchar*)scr->mmio + Pramin + 0x1E00 * 4);
		break;
	default:
		/*
		 * Reset the cursor location, since the kernel may
		 * have allocated less storage than aux/vga
		 * expected.
		 */
		tmp = scr->apsize - 96*1024;
		p = (void*)((uchar*)scr->vaddr + tmp);
		vgaxo(Crtx, 0x30, 0x80|(tmp>>17));
		vgaxo(Crtx, 0x31, (tmp>>11)<<2);
		vgaxo(Crtx, 0x2F, tmp>>24);
		break;
	}

	for(i=0; i<16; i++) {
		c = (curs->clr[2 * i] << 8) | curs->clr[2 * i+1];
		s = (curs->set[2 * i] << 8) | curs->set[2 * i+1];
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

	if(scr->mmio == 0)
		return 1;

	cursorpos = (void*)((uchar*)scr->mmio + hwCurPos);
	*cursorpos = ((p.y+scr->offset.y)<<16)|((p.x+scr->offset.x) & 0xFFFF);

	return 0;
}

static void
nvidiacurenable(VGAscr* scr)
{
	nvidiaenable(scr);
	if(scr->mmio == 0)
		return;

	vgaxo(Crtx, 0x1F, 0x57);

	nvidiacurload(scr, &arrow);
	nvidiacurmove(scr, ZP);

	vgaxo(Crtx, 0x31, vgaxi(Crtx, 0x31) | 0x01);
}

void
writeput(VGAscr *scr, int data)
{
	uchar	*p, scratch;
	ulong	*fifo;

	outb(0x3D0,0);
	p = scr->vaddr;
	scratch = *p;
	fifo = (void*)((uchar*)scr->mmio + Fifo);
	fifo[0x10] = (data << 2);
	USED(scratch);
}

ulong
readget(VGAscr *scr)
{
	ulong	*fifo;

	fifo = (void*)((uchar*)scr->mmio + Fifo);
	return (fifo[0x0011] >> 2);
}

void
nvdmakickoff(VGAscr *scr)
{
	if(nv.dmacurrent != nv.dmaput) {
		nv.dmaput = nv.dmacurrent;
		writeput(scr, nv.dmaput);
	}
}

static void
nvdmanext(ulong data)
{
	nv.dmabase[nv.dmacurrent++] = data;
}

void
nvdmawait(VGAscr *scr, int size)
{
	int dmaget;

	size++;

	while(nv.dmafree < size) {
		dmaget = readget(scr);

		if(nv.dmaput >= dmaget) {
			nv.dmafree = nv.dmamax - nv.dmacurrent;
			if(nv.dmafree < size) {
				nvdmanext(0x20000000);
				if(dmaget <= SKIPS) {
					if (nv.dmaput <= SKIPS) /* corner case - will be idle */
						writeput(scr, SKIPS + 1);
					do { dmaget = readget(scr); }
					while(dmaget <= SKIPS);
				}
				writeput(scr, SKIPS);
				nv.dmacurrent = nv.dmaput = SKIPS;
				nv.dmafree = dmaget - (SKIPS + 1);
			}
		} else
			nv.dmafree = dmaget - nv.dmacurrent - 1;
	}
}


static void
nvdmastart(VGAscr *scr, ulong tag, int size)
{
	if (nv.dmafree <= size)
		nvdmawait(scr, size);
	nvdmanext((size << 18) | tag);
	nv.dmafree -= (size + 1);
}

static void
waitforidle(VGAscr *scr)
{
	ulong*	pgraph;
	int x;

	pgraph = (void*)((uchar*)scr->mmio + Pgraph);

	x = 0;
	while((readget(scr) != nv.dmaput) && x++ < 1000000)
		;
	if(x >= 1000000)
		iprint("idle stat %lud put %d scr %#p pc %#p\n", readget(scr), nv.dmaput, scr, getcallerpc(&scr));

	x = 0;
	while(pgraph[0x00000700/4] & 0x01 && x++ < 1000000)
		;

	if(x >= 1000000)
		iprint("idle stat %lud scrio %#p scr %#p pc %#p\n", *pgraph, scr->mmio, scr, getcallerpc(&scr));
}

static void
nvresetgraphics(VGAscr *scr)
{
	ulong	surfaceFormat, patternFormat, rectFormat, lineFormat;
	int		pitch, i;

	pitch = scr->gscreen->width*BY2WD;

	/*
	 * DMA is at the end of the virtual window,
	 * but we might have cut it short when mapping it.
	 */
	if(nv.dmabase == nil){
		if(scr->storage <= scr->apsize)
			nv.dmabase = (ulong*)((uchar*)scr->vaddr + scr->storage - 128*1024);
		else{
			nv.dmabase = (void*)vmap(scr->paddr + scr->storage - 128*1024, 128*1024);
			if(nv.dmabase == 0){
				hwaccel = 0;
				hwblank = 0;
				print("vmap nvidia dma failed\n");
				return;
			}
		}
	}

	for(i=0; i<SKIPS; i++)
		nv.dmabase[i] = 0x00000000;

	nv.dmabase[0x0 + SKIPS] = 0x00040000;
	nv.dmabase[0x1 + SKIPS] = 0x80000010;
	nv.dmabase[0x2 + SKIPS] = 0x00042000;
	nv.dmabase[0x3 + SKIPS] = 0x80000011;
	nv.dmabase[0x4 + SKIPS] = 0x00044000;
	nv.dmabase[0x5 + SKIPS] = 0x80000012;
	nv.dmabase[0x6 + SKIPS] = 0x00046000;
	nv.dmabase[0x7 + SKIPS] = 0x80000013;
	nv.dmabase[0x8 + SKIPS] = 0x00048000;
	nv.dmabase[0x9 + SKIPS] = 0x80000014;
	nv.dmabase[0xA + SKIPS] = 0x0004A000;
	nv.dmabase[0xB + SKIPS] = 0x80000015;
	nv.dmabase[0xC + SKIPS] = 0x0004C000;
	nv.dmabase[0xD + SKIPS] = 0x80000016;
	nv.dmabase[0xE + SKIPS] = 0x0004E000;
	nv.dmabase[0xF + SKIPS] = 0x80000017;

	nv.dmaput = 0;
	nv.dmacurrent = 16 + SKIPS;
	nv.dmamax = 8191;
	nv.dmafree = nv.dmamax - nv.dmacurrent;

	switch(scr->gscreen->depth) {
	case 32:
	case 24:
		surfaceFormat = SURFACE_FORMAT_DEPTH24;
		patternFormat = PATTERN_FORMAT_DEPTH24;
		rectFormat = RECT_FORMAT_DEPTH24;
		lineFormat = LINE_FORMAT_DEPTH24;
		break;
	case 16:
	case 15:
		surfaceFormat = SURFACE_FORMAT_DEPTH16;
		patternFormat = PATTERN_FORMAT_DEPTH16;
		rectFormat = RECT_FORMAT_DEPTH16;
		lineFormat = LINE_FORMAT_DEPTH16;
		break;
	default:
		surfaceFormat = SURFACE_FORMAT_DEPTH8;
		patternFormat = PATTERN_FORMAT_DEPTH8;
		rectFormat = RECT_FORMAT_DEPTH8;
		lineFormat = LINE_FORMAT_DEPTH8;
		break;
	}

	nvdmastart(scr, SURFACE_FORMAT, 4);
	nvdmanext(surfaceFormat);
	nvdmanext(pitch | (pitch << 16));
	nvdmanext(0);
	nvdmanext(0);

	nvdmastart(scr, PATTERN_FORMAT, 1);
	nvdmanext(patternFormat);

	nvdmastart(scr, RECT_FORMAT, 1);
	nvdmanext(rectFormat);

	nvdmastart(scr, LINE_FORMAT, 1);
	nvdmanext(lineFormat);

	nvdmastart(scr, PATTERN_COLOR_0, 4);
	nvdmanext(~0);
	nvdmanext(~0);
	nvdmanext(~0);
	nvdmanext(~0);

	nvdmastart(scr, ROP_SET, 1);
	nvdmanext(0xCC);

	nvdmakickoff(scr);
	waitforidle(scr);
}


static int
nvidiahwfill(VGAscr *scr, Rectangle r, ulong sval)
{
	nvdmastart(scr, RECT_SOLID_COLOR, 1);
	nvdmanext(sval);

	nvdmastart(scr, RECT_SOLID_RECTS(0), 2);
	nvdmanext((r.min.x << 16) | r.min.y);
	nvdmanext((Dx(r) << 16) | Dy(r));

	//if ( (Dy(r) * Dx(r)) >= 512)
		nvdmakickoff(scr);

	waitforidle(scr);

	return 1;
}

static int
nvidiahwscroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	nvdmastart(scr, BLIT_POINT_SRC, 3);
	nvdmanext((sr.min.y << 16) | sr.min.x);
	nvdmanext((r.min.y << 16) | r.min.x);
	nvdmanext((Dy(r) << 16) | Dx(r));

	//if ( (Dy(r) * Dx(r)) >= 512)
		nvdmakickoff(scr);

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
	nvresetgraphics(scr);
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
