#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Tseng Labs Inc. ET4000 Video Controller.
 */
enum {
	Crtcbx		= 0x217A,	/* Secondary CRT controller */

	Sprite		= 0xE0,
	NSprite		= 0x10,

	Ima		= 0xF0,
	NIma		= 0x08,
};

static void
setkey(void)
{
	outportb(0x3BF, 0x03);
	outportb(0x3D8, 0xA0);
	outportb(0x3CD, 0x00);
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;

	setkey();

	vga->sequencer[0x06] = vgaxi(Seqx, 0x06);
	vga->sequencer[0x07] = vgaxi(Seqx, 0x07);

	for(i = 0x30; i < 0x38; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	vga->crt[0x3F] = vgaxi(Crtx, 0x3F);

	vga->attribute[0x16] = vgaxi(Attrx, 0x16);
	vga->attribute[0x17] = vgaxi(Attrx, 0x17);

	/*
	 * Memory size.
	 */
	switch(vga->crt[0x37] & 0x03){

	case 1:
		vga->vmz = 256*1024;
		break;

	case 2:
		vga->vmz = 512*1024;
		break;

	case 3:
		vga->vmz = 1024*1024;
		break;
	}
	if(strncmp(ctlr->name, "et4000-w32", 10) == 0){
		if(vga->crt[0x32] & 0x80)
			vga->vmz *= 2;
	}
	else if(vga->crt[0x37] & 0x80)
		vga->vmz *= 2;

	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	/*
	 * The ET4000 does not need to have the vertical
	 * timing values divided by 2 for interlace mode.
	 */
	if(vga->mode->interlace == 'v')
		vga->mode->interlace = 'V';

	if(strncmp(ctlr->name, "et4000-w32", 10) == 0)
		ctlr->flag |= Hpclk2x8;
	
	ctlr->flag |= Hclkdiv|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	ulong x;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	if(ctlr->flag & Upclk2x8){
		mode = vga->mode;
		vga->crt[0x00] = ((mode->ht/2)>>3)-5;
		vga->crt[0x01] = ((mode->x/2)>>3)-1;
		vga->crt[0x02] = ((mode->shb/2)>>3)-1;
	
		x = (mode->ehb/2)>>3;
		vga->crt[0x03] = 0x80|(x & 0x1F);
		vga->crt[0x04] = (mode->shs/2)>>3;
		vga->crt[0x05] = ((mode->ehs/2)>>3) & 0x1F;
		if(x & 0x20)
			vga->crt[0x05] |= 0x80;
	}
	/*
	 * Itth a mythtawee.
	 */
	if(vga->crt[0x14] & 0x20)
		vga->crt[0x17] |= 0x08;
	vga->crt[0x17] &= ~0x20;

	vga->crt[0x30] = 0x00;
	vga->crt[0x33] = 0x00;

	/*
	 * Overflow High.
	 */
	vga->crt[0x35] = 0x00;
	if(vga->crt[0x15] & 0x400)
		vga->crt[0x35] |= 0x01;
	if(vga->crt[0x06] & 0x400)
		vga->crt[0x35] |= 0x02;
	if(vga->crt[0x12] & 0x400)
		vga->crt[0x35] |= 0x04;
	if(vga->crt[0x10] & 0x400)
		vga->crt[0x35] |= 0x08;
	if(vga->crt[0x18] & 0x400)
		vga->crt[0x35] |= 0x10;
	if(vga->mode->interlace == 'V')
		vga->crt[0x35] |= 0x80;

	/*
	 * Horizontal Overflow.
	 */
	vga->crt[0x3F] = 0x00;
	if(vga->crt[0x00] & 0x100)
		vga->crt[0x3F] |= 0x01;
	if(vga->crt[0x02] & 0x100)
		vga->crt[0x3F] |= 0x04;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x3F] |= 0x10;
	if(vga->crt[0x13] & 0x100)
		vga->crt[0x3F] |= 0x80;

	/*
	 * Turn off MMU buffers, linear map
	 * and memory-mapped registers.
	 */
	vga->crt[0x36] &= ~0x38;

	if(strncmp(ctlr->name, "et4000-w32", 10) == 0)
		vga->crt[0x37] |= 0x80;

	vga->sequencer[0x06] = 0x00;

	/*
	 * Clock select.
	 */
	if(vga->f[0] > 86000000)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);
	vga->misc &= ~0x0C;
	vga->misc |= (vga->i[0] & 0x03)<<2;
	if(vga->i[0] & 0x04)
		vga->crt[0x34] |= 0x02;
	else
		vga->crt[0x34] &= ~0x02;
	vga->crt[0x31] &= ~0xC0;
	vga->crt[0x31] |= (vga->i[0] & 0x18)<<3;

	vga->sequencer[0x07] &= ~0x41;
	if(vga->d[0] == 4)
		vga->sequencer[0x07] |= 0x01;
	else if(vga->d[0] == 2)
		vga->sequencer[0x07] |= 0x40;

	vga->attribute[0x10] &= ~0x40;
	vga->attribute[0x11] = Pblack;
	vga->attribute[0x16] = 0x80;

	if(ctlr->flag & Upclk2x8)
		vga->attribute[0x16] |= 0x20;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	vgaxo(Crtx, 0x30, vga->crt[0x30]);
	vgaxo(Crtx, 0x31, vga->crt[0x31]);
	vgaxo(Crtx, 0x33, vga->crt[0x33]);
	vgaxo(Crtx, 0x34, vga->crt[0x34]);
	vgaxo(Crtx, 0x35, vga->crt[0x35]);
	vgaxo(Crtx, 0x36, vga->crt[0x36]);
	vgaxo(Crtx, 0x37, vga->crt[0x37]);
	vgaxo(Crtx, 0x3F, vga->crt[0x3F]);

	vgaxo(Seqx, 0x06, vga->sequencer[0x06]);
	vgaxo(Seqx, 0x07, vga->sequencer[0x07]);

	vgaxo(Attrx, 0x16, vga->attribute[0x16]);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;
	ushort shb, vrs, x;

	name = ctlr->name;

	printitem(name, "Seq06");
	printreg(vga->sequencer[0x06]);
	printreg(vga->sequencer[0x07]);

	printitem(name, "Crt30");
	for(i = 0x30; i < 0x38; i++)
		printreg(vga->crt[i]);
	printitem(name, "Crt3F");
	printreg(vga->crt[0x3F]);

	printitem(name, "Attr16");
	printreg(vga->attribute[0x16]);
	printreg(vga->attribute[0x17]);

	if(strncmp(name, "et4000-w32", 10) == 0){
		printitem(name, "SpriteE0");
		for(i = Sprite; i < Sprite+NSprite; i++){
			outportb(Crtcbx, i);
			printreg(inportb(Crtcbx+1));
		}
		printitem(name, "ImaF0");
		for(i = Ima; i < Ima+NIma; i++){
			outportb(Crtcbx, i);
			printreg(inportb(Crtcbx+1));
		}
	}

	/*
	 * Try to disassemble the snarfed values into
	 * understandable numbers.
	 * Only do this if we weren't called after Finit.
	 */
	if(ctlr->flag & Finit)
		return;

	x = (vga->crt[0x01]+1)<<3;
	printitem(name, "hde");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	shb = ((((vga->crt[0x3F] & 0x04)<<6)|vga->crt[0x02])+1)<<3;
	printitem(name, "shb");
	printreg(shb);
	Bprint(&stdout, "%6ud", shb);

	x = (((vga->crt[0x05] & 0x80)>>2)|(vga->crt[0x03] & 0x1F))<<3;
	printitem(name, "ehb");
	printreg(x);
	for(i = 0; x < shb; i++)
		x |= 0x200<<i;
	Bprint(&stdout, "%6ud", x);

	x = ((((vga->crt[0x3F] & 0x01)<<8)|vga->crt[0x00])+5)<<3;
	printitem(name, "ht");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	x = vga->crt[0x12];
	if(vga->crt[0x07] & 0x02)
		x |= 0x100;
	if(vga->crt[0x07] & 0x40)
		x |= 0x200;
	if(vga->crt[0x35] & 0x04)
		x |= 0x400;
	x += 1;
	printitem(name, "vde");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	vrs = vga->crt[0x10];
	if(vga->crt[0x07] & 0x04)
		vrs |= 0x100;
	if(vga->crt[0x07] & 0x80)
		vrs |= 0x200;
	if(vga->crt[0x35] & 0x08)
		vrs |= 0x400;
	printitem(name, "vrs");
	printreg(vrs);
	Bprint(&stdout, "%6ud", vrs);

	x = (vrs & ~0x0F)|(vga->crt[0x11] & 0x0F);
	printitem(name, "vre");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	x = vga->crt[0x06];
	if(vga->crt[0x07] & 0x01)
		x |= 0x100;
	if(vga->crt[0x07] & 0x20)
		x |= 0x200;
	if(vga->crt[0x35] & 0x02)
		x |= 0x400;
	x += 2;
	printitem(name, "vt");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	printitem(name, "d i");
	if(vga->sequencer[0x07] & 0x01)
		x = 4;
	else if(vga->sequencer[0x07] & 0x40)
		x = 2;
	else
		x = 0;
	Bprint(&stdout, "%9ud", x);
	x = (vga->misc & 0x0C)>>2;
	if(vga->crt[0x34] & 0x02)
		x |= 0x04;
	x |= (vga->crt[0x31] & 0xC0)>>3;
	Bprint(&stdout, "%8ud\n", x);
}

Ctlr et4000 = {
	"et4000",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
