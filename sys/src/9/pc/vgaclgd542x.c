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
clgd542xpageset(VGAscr*, int page)
{
	uchar gr09;
	int opage;
	
	if(vgaxi(Seqx, 0x07) & 0xF0)
		page = 0;
	gr09 = vgaxi(Grx, 0x09);
	if(vgaxi(Grx, 0x0B) & 0x20){
		vgaxo(Grx, 0x09, page<<2);
		opage = gr09>>2;
	}
	else{
		vgaxo(Grx, 0x09, page<<4);
		opage = gr09>>4;
	}

	return opage;
}

static void
clgd542xpage(VGAscr* scr, int page)
{
	lock(&scr->devlock);
	clgd542xpageset(scr, page);
	unlock(&scr->devlock);
}

static ulong
clgd542xlinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, oaperture;
	int oapsize, wasupamem;
	Pcidev *p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;
	if(wasupamem)
		upafree(oaperture, oapsize);
	scr->isupamem = 0;

	if(p = pcimatch(nil, 0x1013, 0)){
		aperture = p->mem[0].bar & ~0x0F;
		*size = p->mem[0].size;
	}
	else
		aperture = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0))
			scr->isupamem = 1;
	}
	else
		scr->isupamem = 1;

	return aperture;
}

static void
clgd542xdisable(VGAscr*)
{
	uchar sr12;

	sr12 = vgaxi(Seqx, 0x12);
	vgaxo(Seqx, 0x12, sr12 & ~0x01);
}

static void
clgd542xenable(VGAscr* scr)
{
	uchar sr12;
	int mem, x;
 
	/*
	 * Disable the cursor.
	 */
	sr12 = vgaxi(Seqx, 0x12);
	vgaxo(Seqx, 0x12, sr12 & ~0x01);

	/*
	 * Cursor colours.
	 * Can't call setcolor here as cursor is already locked.
	 */
	vgaxo(Seqx, 0x12, sr12|0x02);
	vgao(PaddrW, 0x00);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(Pdata, Pwhite);
	vgao(PaddrW, 0x0F);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	vgao(Pdata, Pblack);
	vgaxo(Seqx, 0x12, sr12);

	mem = 0;
	switch(vgaxi(Crtx, 0x27) & ~0x03){

	case 0x88:				/* CL-GD5420 */
	case 0x8C:				/* CL-GD5422 */
	case 0x94:				/* CL-GD5424 */
	case 0x80:				/* CL-GD5425 */
	case 0x90:				/* CL-GD5426 */
	case 0x98:				/* CL-GD5427 */
	case 0x9C:				/* CL-GD5429 */
		/*
		 * The BIOS leaves the memory size in Seq0A, bits 4 and 3.
		 * See Technical Reference Manual Appendix E1, Section 1.3.2.
		 *
		 * The storage area for the 64x64 cursors is the last 16Kb of
		 * display memory.
		 */
		mem = (vgaxi(Seqx, 0x0A)>>3) & 0x03;
		break;

	case 0xA0:				/* CL-GD5430 */
	case 0xA8:				/* CL-GD5434 */
	case 0xAC:				/* CL-GD5436 */
	case 0xB8:				/* CL-GD5446 */
	case 0x30:				/* CL-GD7543 */
		/*
		 * Attempt to intuit the memory size from the DRAM control
		 * register. Minimum is 512KB.
		 * If DRAM bank switching is on then there's double.
		 */
		x = vgaxi(Seqx, 0x0F);
		mem = (x>>3) & 0x03;
		if(x & 0x80)
			mem++;
		break;

	default:				/* uh, ah dunno */
		break;
	}
	scr->storage = ((256<<mem)-16)*1024;

	/*
	 * Set the current cursor to index 0
	 * and turn the 64x64 cursor on.
	 */
	vgaxo(Seqx, 0x13, 0);
	vgaxo(Seqx, 0x12, sr12|0x05);
}

static void
clgd542xinitcursor(VGAscr* scr, int xo, int yo, int index)
{
	uchar *p, seq07;
	uint p0, p1;
	int opage, x, y;

	/*
	 * Is linear addressing turned on? This will determine
	 * how we access the cursor storage.
	 */
	seq07 = vgaxi(Seqx, 0x07);
	opage = 0;
	p = KADDR(scr->aperture);
	if(!(seq07 & 0xF0)){
		lock(&scr->devlock);
		opage = clgd542xpageset(scr, scr->storage>>16);
		p += (scr->storage & 0xFFFF);
	}
	else
		p += scr->storage;
	p += index*1024;

	for(y = yo; y < 16; y++){
		p0 = scr->set[2*y];
		p1 = scr->set[2*y+1];
		if(xo){
			p0 = (p0<<xo)|(p1>>(8-xo));
			p1 <<= xo;
		}
		*p++ = p0;
		*p++ = p1;

		for(x = 16; x < 64; x += 8)
			*p++ = 0x00;

		p0 = scr->clr[2*y]|scr->set[2*y];
		p1 = scr->clr[2*y+1]|scr->set[2*y+1];
		if(xo){
			p0 = (p0<<xo)|(p1>>(8-xo));
			p1 <<= xo;
		}
		*p++ = p0;
		*p++ = p1;

		for(x = 16; x < 64; x += 8)
			*p++ = 0x00;
	}
	while(y < 64+yo){
		for(x = 0; x < 64; x += 8){
			*p++ = 0x00;
			*p++ = 0x00;
		}
		y++;
	}

	if(!(seq07 & 0xF0)){
		clgd542xpageset(scr, opage);
		unlock(&scr->devlock);
	}
}

static void
clgd542xload(VGAscr* scr, Cursor* curs)
{
	uchar sr12;

	/*
	 * Disable the cursor.
	 */
	sr12 = vgaxi(Seqx, 0x12);
	vgaxo(Seqx, 0x12, sr12 & ~0x01);

	memmove(&scr->Cursor, curs, sizeof(Cursor));
	clgd542xinitcursor(scr, 0, 0, 0);

	/*
	 * Enable the cursor.
	 */
	vgaxo(Seqx, 0x13, 0);
	vgaxo(Seqx, 0x12, sr12|0x05);
}

static int
clgd542xmove(VGAscr* scr, Point p)
{
	int index, x, xo, y, yo;

	index = 0;
	if((x = p.x+scr->offset.x) < 0){
		xo = -x;
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

	if(xo || yo){
		clgd542xinitcursor(scr, xo, yo, 1);
		index = 1;
	}
	vgaxo(Seqx, 0x13, index<<2);
	
	vgaxo(Seqx, 0x10|((x & 0x07)<<5), (x>>3) & 0xFF);
	vgaxo(Seqx, 0x11|((y & 0x07)<<5), (y>>3) & 0xFF);

	return 0;
}

VGAdev vgaclgd542xdev = {
	"clgd542x",

	0,
	0,
	clgd542xpage,
	clgd542xlinear,
};

VGAcur vgaclgd542xcur = {
	"clgd542xhwgc",

	clgd542xenable,
	clgd542xdisable,
	clgd542xload,
	clgd542xmove,
};
