#include <u.h>
#include <libc.h>

#include "vga.h"

/*
 * Cirrus Logic True Color VGA Family - CL-GD542X.
 * Also works for Alpine VGA Family - CL-GD543X.
 * Just the basics. BUGS:
 *   no hwgc support;
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
	{ 0xA8, 110000000, },		/* CL-GD5434 */
	{ 0x00, },
};

static Gd542x*
identify(Vga *vga)
{
	Gd542x *gd542x;
	uchar id;

	id = vga->crt[0x27] & ~0x03;
	for(gd542x = &family[0]; gd542x->id; gd542x++){
		if(gd542x->id == id)
			return gd542x;
	}

	error("cannot identify CL-GD542X chip - 0x%2.2X\n", vga->crt[0x27]);
	return 0;
}

static void
unlock(void)
{
	vgaxo(Seqx, 0x06, 0x12);
}

static void
snarf(Vga *vga, Ctlr *ctlr)
{
	int i;

	verbose("%s->snarf\n", ctlr->name);

	unlock();

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

	/*
	 * Memory size. The BIOS leaves this in Seq0A, bits 4 and 3.
	 * See Technical Reference Manual Appendix E1, Section 1.3.2.
	 */
	switch((vga->sequencer[0x0A]>>3) & 0x03){

	case 0:
		vga->vmb = 256*1024;
		break;

	case 1:
		vga->vmb = 512*1024;
		break;

	case 2:
		vga->vmb = 1024*1024;
		break;

	case 3:
		vga->vmb = 2048*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
clock(Vga *vga, Ctlr *ctlr)
{
	int f;
	ulong d, dmin, fmin, n, nmin, p;

	verbose("%s->init->clock\n", ctlr->name);

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
	 *	vclk = (Frequency*n)/(d*(1+p));
	 *
	 * There's nothing like brute force and ignorance.
	 */
	fmin = vga->f;
	nmin = 69;
	dmin = 24;
	if(vga->f >= 40000000)
		p = 0;
	else
		p = 1;
	for(n = 1; n < 128; n++){
		for(d = 1; d < 32; d++){
			f = vga->f - (Frequency*n)/(d*(1+p));
			if(f < 0)
				f = -f;
			if(f <= fmin){
				fmin = f;
				nmin = n;
				dmin = d;
			}
		}
	}

	vga->f = (Frequency*nmin)/(dmin*(1+p));
	vga->d = dmin;
	vga->n = nmin;
	vga->p = p;
}

void
init(Vga *vga, Ctlr *ctlr)
{
	Mode *mode;
	Gd542x *gd542x;
	ushort x;

	verbose("%s->init\n", ctlr->name);

	mode = vga->mode;
	gd542x = identify(vga);

	if(vga->f == 0)
		vga->f = vga->mode->frequency;
	if(vga->f > gd542x->vclk)
		error("%s: pclk %d too high\n", ctlr->name, vga->f);

	/*
	 * VCLK3
	 */
	clock(vga, ctlr);
	vga->misc |= 0x0C;
	vga->sequencer[0x0E] = vga->n;
	vga->sequencer[0x1E] = (vga->d<<1)|vga->p;

	vga->sequencer[0x07] = 0x00;
	if(mode->z == 8)
		vga->sequencer[0x07] |= 0x01;

	if(vga->f >= 42000000)
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

	if(mode->interlace == 'v'){
		vga->crt[0x19] = vga->crt[0x00]/2;
		vga->crt[0x1A] |= 0x01;
	}
}

static void
load(Vga *vga, Ctlr *ctlr)
{
	verbose("%s->load\n", ctlr->name);

	vgaxo(Seqx, 0x0E, vga->sequencer[0x0E]);
	vgaxo(Seqx, 0x1E, vga->sequencer[0x1E]);
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
dump(Vga *vga, Ctlr *ctlr)
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
