#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Cirrus Logic True Color VGA Family - CL-GD542X.
 * Also works for Alpine VGA Family - CL-GD543X.
 * Just the basics. BUGS:
 *   the added capabilities of the 543X aren't used.
 */

typedef struct {
	uchar	id;			/* Id */
	ulong	vclk;			/* Maximum dot clock */
} Gd542x;

static Gd542x family[] = {
	{ 0x88,  75000000, },		/* CL-GD5420 */
	{ 0x8C,  80000000, },		/* CL-GD5422 */
	{ 0x94,  80000000, },		/* CL-GD5424 */
	{ 0x90,  80000000, },		/* CL-GD5426 */
	{ 0x98,  80000000, },		/* CL-GD5428 */
	{ 0x9C,  86000000, },		/* CL-GD5429 */
	{ 0xA0,  86000000, },		/* CL-GD5430 */
	{ 0xA8,  86000000, },		/* CL-GD5434 */

	{ 0xAC, 135000000, },		/* CL-GD5436 */
	{ 0xB8, 135000000, },		/* CL-GD5446 */
	{ 0xBC, 135000000, },		/* CL-GD5480 */

	{ 0x30,  80000000, },		/* CL-GD7543 */

	{ 0x00, },
};

static Gd542x*
identify(Vga* vga, Ctlr* ctlr)
{
	Gd542x *gd542x;
	uchar id;

	id = vga->crt[0x27] & ~0x03;
	for(gd542x = &family[0]; gd542x->id; gd542x++){
		if(gd542x->id == id)
			return gd542x;
	}

	error("%s: unknown chip id - 0x%2.2X\n", ctlr->name, vga->crt[0x27]);
	return 0;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Gd542x *gd542x;

	/*
	 * Unlock extended registers.
	 */
	vgaxo(Seqx, 0x06, 0x12);

	/*
	 * Save all the registers, even though we'll only
	 * change a handful.
	 */
	for(i = 0x06; i < 0x20; i++)
		vga->sequencer[i] = vgaxi(Seqx, i);

	for(i = 0x09; i < 0x3A; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	for(i = 0x19; i < 0x1E; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	vga->crt[0x27] = vgaxi(Crtx, 0x27);

	/*
	 * Hack for Hidden DAC Register. Do 4 dummy reads
	 * of Pixmask first.
	 */
	for(i = 0; i < 4; i++)
		vgai(Pixmask);
	vga->crt[0x28] = vgai(Pixmask);

	i = 0;
	switch(vga->crt[0x27] & ~0x03){

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
		i = (vga->sequencer[0x0A]>>3) & 0x03;
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
		i = (vga->sequencer[0x0F]>>3) & 0x03;
		if(vga->sequencer[0x0F] & 0x80)
			i++;

		/*
		 * If it's a PCI card, can do linear.
		 * Most of the Cirrus chips can do linear addressing with
		 * all the different buses, but it can get messy. It's easy
		 * to cut PCI on the CLGD543x chips out of the pack.
		 */
		if(((vga->sequencer[0x17]>>3) & 0x07) == 0x04)
			ctlr->flag |= Hlinear;
		break;
	case 0xBC:				/* CL-GD5480 */
		i = 2;				/* 1024 = 256<<2 */
		if((vga->sequencer[0x0F] & 0x18) == 0x18){
			i <<= 1;		/* 2048 = 256<<3 */
			if(vga->sequencer[0x0F] & 0x80)
				i <<= 2;	/* 2048 = 256<<4 */
		}
		if(vga->sequencer[0x17] & 0x80)
			i <<= 1;
		ctlr->flag |= Hlinear;
		break;

	default:				/* uh, ah dunno */
		break;
	}

	if(vga->linear && (ctlr->flag & Hlinear)){
		vga->vmz = 16*1024*1024;
		vga->vma = 16*1024*1024;
		ctlr->flag |= Ulinear;
	}
	else
		vga->vmz = (256<<i)*1024;

	gd542x = identify(vga, ctlr);
	if(vga->f[1] == 0 || vga->f[1] > gd542x->vclk)
		vga->f[1] = gd542x->vclk;
	ctlr->flag |= Fsnarf;
}

void
clgd54xxclock(Vga* vga, Ctlr* ctlr)
{
	int f;
	ulong d, dmin, fmin, n, nmin, p;

	trace("%s->init->clgd54xxclock\n", ctlr->name);

	/*
	 * Athough the Technical Reference Manual says only a handful
	 * of frequencies are tested, it also gives the following formula
	 * which can be used to generate any frequency within spec.,
	 * including the tested ones.
	 *
	 * Numerator is 7-bits, denominator 5-bits.
	 * Guess from the Technical Reference Manual that
	 * The post divisor is 1 for vclk<40MHz.
	 *
	 * Look for values of n and d and p that give
	 * the least error for
	 *	vclk = (RefFreq*n)/(d*(1+p));
	 *
	 * There's nothing like brute force and ignorance.
	 */
	fmin = vga->f[0];
	nmin = 69;
	dmin = 24;
	if(vga->f[0] >= 40000000)
		p = 0;
	else
		p = 1;
	for(n = 1; n < 128; n++){
		for(d = 1; d < 32; d++){
			f = vga->f[0] - (RefFreq*n)/(d*(1+p));
			if(f < 0)
				f = -f;
			if(f <= fmin){
				fmin = f;
				nmin = n;
				dmin = d;
			}
		}
	}

	vga->f[0] = (RefFreq*nmin)/(dmin*(1+p));
	vga->d[0] = dmin;
	vga->n[0] = nmin;
	vga->p[0] = p;
}

void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	Gd542x *gd542x;
	ushort x;

	mode = vga->mode;
	gd542x = identify(vga, ctlr);

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > gd542x->vclk)
		error("%s: pclk %lud too high (> %lud)\n",
			ctlr->name, vga->f[0], gd542x->vclk);

	if(mode->z > 8)
		error("%s: depth %d not supported\n", ctlr->name, mode->z);

	/*
	 * VCLK3
	 */
	clgd54xxclock(vga, ctlr);
	vga->misc |= 0x0C;
	vga->sequencer[0x0E] = vga->n[0];
	vga->sequencer[0x1E] = (vga->d[0]<<1)|vga->p[0];

	vga->sequencer[0x07] = 0x00;
	if(mode->z == 8)
		vga->sequencer[0x07] |= 0x01;

	if(vga->f[0] >= 42000000)
		vga->sequencer[0x0F] |= 0x20;
	else
		vga->sequencer[0x0F] &= ~0x20;

	vga->sequencer[0x16] = (vga->sequencer[0x16] & 0xF0)|0x08;

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

	vga->graphics[0x0B] = 0x00;
	if(vga->vmz > 1024*1024)
		vga->graphics[0x0B] |= 0x20;

	if(mode->interlace == 'v'){
		vga->crt[0x19] = vga->crt[0x00]/2;
		vga->crt[0x1A] |= 0x01;
	}
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	vgaxo(Seqx, 0x0E, vga->sequencer[0x0E]);
	vgaxo(Seqx, 0x1E, vga->sequencer[0x1E]);
	if(ctlr->flag & Ulinear)
		vga->sequencer[0x07] |= 0xE0;
	vgaxo(Seqx, 0x07, vga->sequencer[0x07]);
	vgaxo(Seqx, 0x0F, vga->sequencer[0x0F]);
	vgaxo(Seqx, 0x16, vga->sequencer[0x16]);

	if(vga->mode->interlace == 'v')
		vgaxo(Crtx, 0x19, vga->crt[0x19]);
	vgaxo(Crtx, 0x1A, vga->crt[0x1A]);
	vgaxo(Crtx, 0x1B, vga->crt[0x1B]);

	vgaxo(Grx, 0x0B, vga->graphics[0x0B]);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;

	name = ctlr->name;

	printitem(name, "Seq06");
	for(i = 0x06; i < 0x20; i++)
		printreg(vga->sequencer[i]);

	printitem(name, "Crt19");
	for(i = 0x19; i < 0x1E; i++)
		printreg(vga->crt[i]);

	printitem(name, "Gr09");
	for(i = 0x09; i < 0x3A; i++)
		printreg(vga->graphics[i]);

	printitem(name, "Id Hdr");
	printreg(vga->crt[0x27]);
	printreg(vga->crt[0x28]);
}

Ctlr clgd542x = {
	"clgd542x",			/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr clgd542xhwgc = {
	"clgd542xhwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
