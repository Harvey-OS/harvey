#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

enum {
	NSeqx		= 0x05,
	NCrtx		= 0x19,
	NGrx		= 0x09,
	NAttrx		= 0x15,
};

uchar
vgai(long port)
{
	uchar data;

	switch(port){

	case MiscR:
	case Status0:
	case Status1:
	case FeatureR:
	case PaddrW:
	case Pdata:
	case Pixmask:
	case Pstatus:
		data = inportb(port);
		break;

	default:
		error("vgai(0x%4.4lX): unknown port\n", port);
		/*NOTREACHED*/
		data = 0xFF;
		break;
	}
	return data;
}

uchar
vgaxi(long port, uchar index)
{
	uchar data;

	switch(port){

	case Seqx:
	case Crtx:
	case Grx:
		outportb(port, index);
		data = inportb(port+1);
		break;

	case Attrx:
		/*
		 * Allow processor access to the colour
		 * palette registers. Writes to Attrx must
		 * be preceded by a read from Status1 to
		 * initialise the register to point to the
		 * index register and not the data register.
		 * Processor access is allowed by turning
		 * off bit 0x20.
		 */
		inportb(Status1);
		if(index < 0x10){
			outportb(Attrx, index);
			data = inportb(Attrx+1);
			inportb(Status1);
			outportb(Attrx, 0x20|index);
		}
		else{
			outportb(Attrx, 0x20|index);
			data = inportb(Attrx+1);
		}
		break;

	default:
		error("vgaxi(0x%4.4lx, 0x%2.2uX): unknown port\n", port, index);
		/*NOTREACHED*/
		data = 0xFF;
		break;
	}
	return data;
}

void
vgao(long port, uchar data)
{
	switch(port){

	case MiscW:
	case FeatureW:
	case PaddrW:
	case Pdata:
	case Pixmask:
	case PaddrR:
		outportb(port, data);
		break;

	default:
		error("vgao(0x%4.4lX, 0x%2.2uX): unknown port\n", port, data);
		/*NOTREACHED*/
		break;
	}
}

void
vgaxo(long port, uchar index, uchar data)
{
	switch(port){

	case Seqx:
	case Crtx:
	case Grx:
		/*
		 * We could use an outport here, but some chips
		 * (e.g. 86C928) have trouble with that for some
		 * registers.
		 */
		outportb(port, index);
		outportb(port+1, data);
		break;

	case Attrx:
		inportb(Status1);
		if(index < 0x10){
			outportb(Attrx, index);
			outportb(Attrx, data);
			inportb(Status1);
			outportb(Attrx, 0x20|index);
		}
		else{
			outportb(Attrx, 0x20|index);
			outportb(Attrx, data);
		}
		break;

	default:
		error("vgaxo(0x%4.4lX, 0x%2.2uX, 0x%2.2uX): unknown port\n",
			port, index, data);
		break;
	}
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;

	/*
	 * Generic VGA registers:
	 * 	misc, feature;
	 *	sequencer;
	 *	crt;
	 *	graphics;
	 *	attribute;
	 *	palette.
	 */
	vga->misc = vgai(MiscR);
	vga->feature = vgai(FeatureR);

	for(i = 0; i < NSeqx; i++)
		vga->sequencer[i] = vgaxi(Seqx, i);

	for(i = 0; i < NCrtx; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	for(i = 0; i < NGrx; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	for(i = 0; i < NAttrx; i++)
		vga->attribute[i] = vgaxi(Attrx, i);

	if(dflag)
		palette.snarf(vga, ctlr);

	ctlr->flag |= Fsnarf;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	int vt, vde, vrs, vre;
	ulong tmp;

	mode = vga->mode;

	memset(vga->sequencer, 0, NSeqx*sizeof(vga->sequencer[0]));
	memset(vga->crt, 0, NCrtx*sizeof(vga->crt[0]));
	memset(vga->graphics, 0, NGrx*sizeof(vga->graphics[0]));
	memset(vga->attribute, 0, NAttrx*sizeof(vga->attribute[0]));
	if(dflag)
		memset(vga->palette, 0, sizeof(vga->palette));

	/*
	 * Misc. If both the horizontal and vertical sync polarity
	 * options are set, use them. Otherwise use the defaults for
	 * the given vertical size.
	 */
	vga->misc = 0x23;
	if(mode->frequency == VgaFreq1)
		vga->misc |= 0x04;
	if(mode->hsync && mode->vsync){
		if(mode->hsync == '-')
			vga->misc |= 0x40;
		if(mode->vsync == '-')
			vga->misc |= 0x80;
	}
	else{
		if(mode->y < 480)
			vga->misc |= 0x40;
		else if(mode->y < 400)
			vga->misc |= 0x80;
		else if(mode->y < 768)
			vga->misc |= 0xC0;
	}

	/*
	 * Sequencer
	 */
	vga->sequencer[0x00] = 0x03;
	vga->sequencer[0x01] = 0x01;
	vga->sequencer[0x02] = 0x0F;
	vga->sequencer[0x03] = 0x00;
	if(mode->z >= 8)
		vga->sequencer[0x04] = 0x0A;
	else
		vga->sequencer[0x04] = 0x06;

	/*
	 * Crt. Most of the work here is in dealing
	 * with field overflow.
	 */
	memset(vga->crt, 0, NCrtx);

	vga->crt[0x00] = (mode->ht>>3)-5;
	vga->crt[0x01] = (mode->x>>3)-1;
	vga->crt[0x02] = (mode->shb>>3)-1;

	/*
	 * End Horizontal Blank is a 6-bit field, 5-bits
	 * in Crt3, high bit in Crt5.
	 */
	tmp = mode->ehb>>3;
	vga->crt[0x03] = 0x80|(tmp & 0x1F);
	if(tmp & 0x20)
		vga->crt[0x05] |= 0x80;

	if(mode->shs == 0)
		mode->shs = mode->shb;
	vga->crt[0x04] = mode->shs>>3;
	if(mode->ehs == 0)
		mode->ehs = mode->ehb;
	vga->crt[0x05] |= ((mode->ehs>>3) & 0x1F);

	/*
	 * Vertical Total is 10-bits, 8 in Crt6, the high
	 * two bits in Crt7. What if vt is >= 1024? We hope
	 * the specific controller has some more overflow
	 * bits.
	 *
	 * Interlace: if 'v',  divide the vertical timing
	 * values by 2.
	 */
	vt = mode->vt;
	vde = mode->y;
	vrs = mode->vrs;
	vre = mode->vre;

	if(mode->interlace == 'v'){
		vt /= 2;
		vde /= 2;
		vrs /= 2;
		vre /= 2;
	}

	tmp = vt-2;
	vga->crt[0x06] = tmp;
	if(tmp & 0x100)
		vga->crt[0x07] |= 0x01;
	if(tmp & 0x200)
		vga->crt[0x07] |= 0x20;

	tmp = vrs;
	vga->crt[0x10] = tmp;
	if(tmp & 0x100)
		vga->crt[0x07] |= 0x04;
	if(tmp & 0x200)
		vga->crt[0x07] |= 0x80;

	vga->crt[0x11] = 0x20|(vre & 0x0F);

	tmp = vde-1;
	vga->crt[0x12] = tmp;
	if(tmp & 0x100)
		vga->crt[0x07] |= 0x02;
	if(tmp & 0x200)
		vga->crt[0x07] |= 0x40;

	vga->crt[0x15] = vrs;
	if(vrs & 0x100)
		vga->crt[0x07] |= 0x08;
	if(vrs & 0x200)
		vga->crt[0x09] |= 0x20;

	vga->crt[0x16] = (vrs+1);

	vga->crt[0x17] = 0x83;
	tmp = ((vga->virtx*mode->z)/8);
	if(tmp >= 512){
		vga->crt[0x14] |= 0x60;
		tmp /= 8;
	}
	else if(tmp >= 256){
		vga->crt[0x17] |= 0x08;
		tmp /= 4;
	}
	else{
		vga->crt[0x17] |= 0x40;
		tmp /= 2;
	}
	vga->crt[0x13] = tmp;

	if(mode->x*mode->y*mode->z/8 > 64*1024)
		vga->crt[0x17] |= 0x20;

	vga->crt[0x18] = 0x7FF;
	if(vga->crt[0x18] & 0x100)
		vga->crt[0x07] |= 0x10;
	if(vga->crt[0x18] & 0x200)
		vga->crt[0x09] |= 0x40;

	/*
	 * Graphics
	 */
	memset(vga->graphics, 0, NGrx);
	if((vga->sequencer[0x04] & 0x04) == 0)
		vga->graphics[0x05] |= 0x10;
	if(mode->z >= 8)
		vga->graphics[0x05] |= 0x40;
	vga->graphics[0x06] = 0x05;
	vga->graphics[0x07] = 0x0F;
	vga->graphics[0x08] = 0xFF;

	/*
	 * Attribute
	 */
	memset(vga->attribute, 0, NAttrx);
	for(tmp = 0; tmp < 0x10; tmp++)
		vga->attribute[tmp] = tmp;
	vga->attribute[0x10] = 0x01;
	if(mode->z >= 8)
		vga->attribute[0x10] |= 0x40;
	vga->attribute[0x11] = 0xFF;
	vga->attribute[0x12] = 0x0F;

	/*
	 * Palette
	 */
	if(dflag)
		palette.init(vga, ctlr);

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int i;

	/*
	 * Reset the sequencer and leave it off.
	 * Load the generic VGA registers:
	 *	misc;
	 *	sequencer (but not seq01, display enable);
	 *	take the sequencer out of reset;
	 *	take off write-protect on crt[0x00-0x07];
	 *	crt;
	 *	graphics;
	 *	attribute;
	 *	palette.
	vgaxo(Seqx, 0x00, 0x00);
	 */

	vgao(MiscW, vga->misc);

	for(i = 2; i < NSeqx; i++)
		vgaxo(Seqx, i, vga->sequencer[i]);
	/*vgaxo(Seqx, 0x00, 0x03);*/

	vgaxo(Crtx, 0x11, vga->crt[0x11] & ~0x80);
	for(i = 0; i < NCrtx; i++)
		vgaxo(Crtx, i, vga->crt[i]);

	for(i = 0; i < NGrx; i++)
		vgaxo(Grx, i, vga->graphics[i]);

	for(i = 0; i < NAttrx; i++)
		vgaxo(Attrx, i, vga->attribute[i]);

	if(dflag)
		palette.load(vga, ctlr);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "misc");
	printreg(vga->misc);
	printitem(ctlr->name, "feature");
	printreg(vga->feature);

	printitem(ctlr->name, "sequencer");
	for(i = 0; i < NSeqx; i++)
		printreg(vga->sequencer[i]);

	printitem(ctlr->name, "crt");
	for(i = 0; i < NCrtx; i++)
		printreg(vga->crt[i]);

	printitem(ctlr->name, "graphics");
	for(i = 0; i < NGrx; i++)
		printreg(vga->graphics[i]);

	printitem(ctlr->name, "attribute");
	for(i = 0; i < NAttrx; i++)
		printreg(vga->attribute[i]);

	if(dflag)
		palette.dump(vga, ctlr);

	printitem(ctlr->name, "virtual");
	Bprint(&stdout, "%ld %ld\n", vga->virtx, vga->virty);
	printitem(ctlr->name, "panning");
	Bprint(&stdout, "%s\n", vga->panning ? "on" : "off");
	if(vga->f[0]){
		printitem(ctlr->name, "clock[0] f");
		Bprint(&stdout, "%9ld\n", vga->f[0]);
		printitem(ctlr->name, "clock[0] d i m");
		Bprint(&stdout, "%9ld %8ld       - %8ld\n",
			vga->d[0], vga->i[0], vga->m[0]);
		printitem(ctlr->name, "clock[0] n p q r");
		Bprint(&stdout, "%9ld %8ld       - %8ld %8ld\n",
			vga->n[0], vga->p[0], vga->q[0], vga->r[0]);
	}
	if(vga->f[1]){
		printitem(ctlr->name, "clock[1] f");
		Bprint(&stdout, "%9ld\n", vga->f[1]);
		printitem(ctlr->name, "clock[1] d i m");
		Bprint(&stdout, "%9ld %8ld       - %8ld\n",
			vga->d[1], vga->i[1], vga->m[1]);
		printitem(ctlr->name, "clock[1] n p q r");
		Bprint(&stdout, "%9ld %8ld       - %8ld %8ld\n",
			vga->n[1], vga->p[1], vga->q[1], vga->r[1]);
	}

	if(vga->vma || vga->vmb){
		printitem(ctlr->name, "vm a b");
		Bprint(&stdout, "%9lud %8lud\n", vga->vma, vga->vmb);
	}
	if(vga->vmz){
		printitem(ctlr->name, "vmz");
		Bprint(&stdout, "%9lud\n", vga->vmz);
	}
	printitem(ctlr->name, "apz");
	Bprint(&stdout, "%9lud\n", vga->apz);

	printitem(ctlr->name, "linear");
	Bprint(&stdout, "%9d\n", vga->linear);
}

Ctlr generic = {
	"vga",				/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
