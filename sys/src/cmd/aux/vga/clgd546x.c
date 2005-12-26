#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Laguna Visual Media Accelerators Family CL-GD546x.
 */
typedef struct {
	Pcidev*	pci;
	uchar*	mmio;
	int	mem;

	int	format;			/* graphics and video format */
	int	threshold;		/* display threshold */
	int	tilectrl;		/* tiling control */
	int	vsc;			/* vendor specific control */
	int	control;		/* control */
	int	tilectrl2D3D;		/* tiling control 2D3D */
} Laguna;

enum {
	Format		= 0xC0,		/* graphics and video format */

	Threshold	= 0xEA,		/* Display Threshold */

	TileCtrl	= 0x2C4,

	Vsc		= 0x3FC,	/* Vendor Specific Control (32-bits) */

	Control		= 0x402,	/* 2D Control */
	TileCtrl2D3D	= 0x407,	/* (8-bits) */
};

static int
mmio8r(Laguna* laguna, int offset)
{
	return *(laguna->mmio+offset) & 0xFF;
}

static void
mmio8w(Laguna* laguna, int offset, int data)
{
	*(laguna->mmio+offset) = data;
}

static int
mmio16r(Laguna* laguna, int offset)
{
	return *((ushort*)(laguna->mmio+offset)) & 0xFFFF;
}

static void
mmio16w(Laguna* laguna, int offset, int data)
{
	*((ushort*)(laguna->mmio+offset)) = data;
}

static int
mmio32r(Laguna* laguna, int offset)
{
	return *((ulong*)(laguna->mmio+offset));
}

static void
mmio32w(Laguna* laguna, int offset, int data)
{
	*((ulong*)(laguna->mmio+offset)) = data;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int f, i;
	uchar *mmio;
	Pcidev *p;
	Laguna *laguna;

	/*
	 * Save all the registers, even though we'll only
	 * change a handful.
	 */
	for(i = 0x06; i < 0x20; i++)
		vga->sequencer[i] = vgaxi(Seqx, i);

	for(i = 0x09; i < 0x0C; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	for(i = 0x19; i < 0x20; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	if(vga->private == nil){
		vga->private = alloc(sizeof(Laguna));
		if((p = pcimatch(0, 0x1013, 0)) == nil)
			error("%s: not found\n", ctlr->name);
		switch(p->did){
		case 0xD0:			/* CL-GD5462 */
			vga->f[1] = 170000000;
			break;
		case 0xD4:			/* CL-GD5464 */
		case 0xD6:			/* CL-GD5465 */
			vga->f[1] = 230000000;
			break;
		default:
			error("%s: not found\n", ctlr->name);
		}

		if((f = open("#v/vgactl", OWRITE)) < 0)
			error("%s: can't open vgactl\n", ctlr->name);
		if(write(f, "type clgd546x", 13) != 13)
			error("%s: can't set type\n", ctlr->name);
		close(f);
	
		mmio = segattach(0, "clgd546xmmio", 0, p->mem[1].size);
		if(mmio == (void*)-1)
			error("%s: can't attach mmio segment\n", ctlr->name);
		laguna = vga->private;
		laguna->pci = p;
		laguna->mmio = mmio;
	}
	laguna = vga->private;

	laguna->mem = (vga->sequencer[0x14] & 0x07)+1;

	laguna->format = mmio16r(laguna, Format);
	laguna->threshold = mmio16r(laguna, Threshold);
	laguna->tilectrl = mmio16r(laguna, TileCtrl);
	laguna->vsc = mmio32r(laguna, Vsc);
	laguna->control = mmio16r(laguna, Control);
	laguna->tilectrl2D3D = mmio8r(laguna, TileCtrl2D3D);

	vga->vma = vga->vmz = laguna->pci->mem[0].size;
	ctlr->flag |= Hlinear;

	ctlr->flag |= Fsnarf;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	ushort x;
	int format, interleave, fetches, nointerleave, notile, pagesize, tiles;
	Laguna *laguna;

	nointerleave = 1;
	notile = 1;
	pagesize = 0;

	mode = vga->mode;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > vga->f[1])
		error("%s: invalid pclk - %lud\n", ctlr->name, vga->f[0]);

	if(mode->z > 8)
		error("%s: depth %d not supported\n", ctlr->name, mode->z);

	/*
	 * VCLK3
	 */
	clgd54xxclock(vga, ctlr);
	vga->misc |= 0x0C;
	vga->sequencer[0x1E] = vga->n[0];
	vga->sequencer[0x0E] = (vga->d[0]<<1)|vga->p[0];

	vga->sequencer[0x07] = 0x00;
	if(mode->z == 8)
		vga->sequencer[0x07] |= 0x01;

	vga->crt[0x14] = 0;
	vga->crt[0x17] = 0xC3;

	/*
	 * Overflow bits.
	 */
	vga->crt[0x1A] = 0x00;
	x = mode->ehb>>3;
	if(x & 0x40)
		vga->crt[0x1A] |= 0x10;
	if(x & 0x80)
		vga->crt[0x1A] |= 0x20;
	if(vga->crt[0x16] & 0x100)
		vga->crt[0x1A] |= 0x40;
	if(vga->crt[0x16] & 0x200)
		vga->crt[0x1A] |= 0x80;
	vga->crt[0x1B] = 0x22;
	if(vga->crt[0x13] & 0x100)
		vga->crt[0x1B] |= 0x10;
	vga->crt[0x1D] = 0x00;
	if(vga->crt[0x13] & 0x200)
		vga->crt[0x1D] |= 0x01;
	vga->crt[0x1E] = 0x00;
	if(vga->crt[0x10] & 0x400)
		vga->crt[0x1E] |= 0x01;
	if(vga->crt[0x15] & 0x400)
		vga->crt[0x1E] |= 0x02;
	if(vga->crt[0x12] & 0x400)
		vga->crt[0x1E] |= 0x04;
	if(vga->crt[0x06] & 0x400)
		vga->crt[0x1E] |= 0x08;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x1E] |= 0x10;
	if(vga->crt[0x02] & 0x100)
		vga->crt[0x1E] |= 0x20;
	if(vga->crt[0x01] & 0x100)
		vga->crt[0x1E] |= 0x40;
	if(vga->crt[0x00] & 0x100)
		vga->crt[0x1E] |= 0x80;

	vga->graphics[0x0B] = 0x00;
	if(vga->vmz > 1024*1024)
		vga->graphics[0x0B] |= 0x20;

	if(mode->interlace == 'v'){
		vga->crt[0x19] = vga->crt[0x00]/2;
		vga->crt[0x1A] |= 0x01;
	}

	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	laguna = vga->private;

	/*
	 * Ignore wide tiles for now, this simplifies things.
	 */
	if(mode->x <= 640)
		tiles = 5;
	else if(mode->x <= 1024)
		tiles = 8;
	else if(mode->x <= 1280)
		tiles = 10;
	else if(mode->x <= 1664)
		tiles = 13;
	else if(mode->x <= 2048)
		tiles = 16;
	else if(mode->x <= 2560)
		tiles = 20;
	else if(mode->x <= 3228)
		tiles = 26;
	else
		tiles = 32;
	fetches = tiles;		/* -1? */

	if(nointerleave)
		interleave = 0;
	else switch(laguna->mem){
	default:
		interleave = 0;
		break;
	case 2:
		interleave = 1;
		break;
	case 4:
	case 8:
		interleave = 2;
		break;
	}

	if(mode->z == 8)
		format = 0;
	else if(mode->z == 16)
		format = (1<<12)|(2<<9);
	else if(mode->z == 24)
		format = (2<<12)|(2<<9);
	else
		format = (2<<12)|(2<<9);

	//if(ctlr->flag & Ulinear)
	//	laguna->vsc |= 0x10000000;
	//else
		laguna->vsc &= ~0x10000000;
	laguna->format = format;
	laguna->threshold = (interleave<<14)|(fetches<<8)|0x14;
	laguna->tilectrl &= 0x3F;
	laguna->tilectrl |= (interleave<<14)|(tiles<<8);
	if(!notile)
		laguna->tilectrl |= 0x80;
	if(pagesize == 1)
		laguna->tilectrl |= 0x10;
	laguna->tilectrl2D3D = (interleave<<6)|tiles;
	laguna->control = 0;
	if(notile)
		laguna->control |= 0x1000;
	if(pagesize == 1)
		laguna->control |= 0x0200;
}

static void
load(Vga* vga, Ctlr*)
{
	Laguna *laguna;

	vgaxo(Seqx, 0x0E, vga->sequencer[0x0E]);
	vgaxo(Seqx, 0x1E, vga->sequencer[0x1E]);
	vgaxo(Seqx, 0x07, vga->sequencer[0x07]);

	if(vga->mode->interlace == 'v')
		vgaxo(Crtx, 0x19, vga->crt[0x19]);
	vgaxo(Crtx, 0x1A, vga->crt[0x1A]);
	vgaxo(Crtx, 0x1B, vga->crt[0x1B]);
	vgaxo(Crtx, 0x1D, vga->crt[0x1D]);
	vgaxo(Crtx, 0x1E, vga->crt[0x1E]);

	vgaxo(Grx, 0x0B, vga->graphics[0x0B]);

	laguna = vga->private;
	mmio16w(laguna, Format, laguna->format);
	mmio32w(laguna, Vsc, laguna->vsc);
	mmio16w(laguna, Threshold, laguna->threshold);
	mmio16w(laguna, TileCtrl, laguna->tilectrl);
	mmio8w(laguna, TileCtrl2D3D, laguna->tilectrl2D3D);
	mmio16w(laguna, Control, laguna->control);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;
	Laguna *laguna;

	name = ctlr->name;

	printitem(name, "Seq06");
	for(i = 0x06; i < 0x20; i++)
		printreg(vga->sequencer[i]);

	printitem(name, "Crt19");
	for(i = 0x19; i < 0x20; i++)
		printreg(vga->crt[i]);

	printitem(name, "Gr09");
	for(i = 0x09; i < 0x0C; i++)
		printreg(vga->graphics[i]);

	laguna = vga->private;
	Bprint(&stdout, "\n");
	Bprint(&stdout, "%s mem\t\t%d\n", ctlr->name, laguna->mem*1024*1024);
	Bprint(&stdout, "%s Format\t\t%uX\n", ctlr->name, laguna->format);
	Bprint(&stdout, "%s Threshold\t\t\t%uX\n",
		ctlr->name, laguna->threshold);
	Bprint(&stdout, "%s TileCtrl\t\t\t%uX\n", ctlr->name, laguna->tilectrl);
	Bprint(&stdout, "%s Vsc\t\t%uX\n", ctlr->name, laguna->vsc);
	Bprint(&stdout, "%s Control\t\t%uX\n", ctlr->name, laguna->control);
	Bprint(&stdout, "%s TileCtrlC2D3D\t\t%uX\n",
		ctlr->name, laguna->tilectrl2D3D);
}

Ctlr clgd546x = {
	"clgd546x",			/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr clgd546xhwgc = {
	"clgd546xhwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
