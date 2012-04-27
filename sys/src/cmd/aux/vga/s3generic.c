#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Generic S3 GUI Accelerator.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;

	trace("%s->snarf->s3generic\n", ctlr->name);

	/*
	 * Unlock extended registers.
	 * 0xA5 ensures Crt36 and Crt37 are also unlocked
	 * (0xA0 unlocks everything else).
	 */
	vgaxo(Crtx, 0x38, 0x48);
	vgaxo(Crtx, 0x39, 0xA5);

	/*
	 * Not all registers exist on all chips.
	 * Crt3[EF] don't exist on any.
	 */
	for(i = 0x30; i < 0x70; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	/*
	 * Memory size.
	 */
	switch((vga->crt[0x36]>>5) & 0x07){

	case 0x00:
		vga->vmz = 4*1024*1024;
		break;

	case 0x02:
		vga->vmz = 3*1024*1024;
		break;

	case 0x04:
		vga->vmz = 2*1024*1024;
		break;

	case 0x06:
		vga->vmz = 1*1024*1024;
		break;

	case 0x07:
		vga->vmz = 512*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	ulong x;

	trace("%s->init->s3generic\n", ctlr->name);
	mode = vga->mode;

	/*
	 * Is enhanced mode is necessary?
	 */
	if((ctlr->flag & (Uenhanced|Henhanced)) == Henhanced){
		if(mode->z >= 8)
			resyncinit(vga, ctlr, Uenhanced, 0);
		else
			resyncinit(vga, ctlr, 0, Uenhanced|Henhanced);
	}
	if((ctlr->flag & Uenhanced) == 0 && mode->x > 1024)
		error("%s: no support for 1-bit mode > 1024x768x1\n", ctlr->name);

	vga->crt[0x31] = 0x85;
	vga->crt[0x6A] &= 0xC0;
	vga->crt[0x32] &= ~0x40;

	vga->crt[0x31] |= 0x08;
	vga->crt[0x32] |= 0x40;

	vga->crt[0x33] |= 0x20;
	if(mode->z >= 8)
		vga->crt[0x3A] |= 0x10;
	else
		vga->crt[0x3A] &= ~0x10;

	vga->crt[0x34] = 0x10;
	vga->crt[0x35] = 0x00;
	if(mode->interlace){
		vga->crt[0x3C] = vga->crt[0]/2;
		vga->crt[0x42] |= 0x20;
	}
	else{
		vga->crt[0x3C] = 0x00;
		vga->crt[0x42] &= ~0x20;
	}

	vga->crt[0x40] = (vga->crt[0x40] & 0xF2);
	vga->crt[0x43] = 0x00;
	vga->crt[0x45] = 0x00;

	vga->crt[0x50] &= 0x3E;
	if(mode->x <= 640)
		x = 0x40;
	else if(mode->x <= 800)
		x = 0x80;
	else if(mode->x <= 1024)
		x = 0x00;
	else if(mode->x <= 1152)
		x = 0x01;
	else if(mode->x <= 1280)
		x = 0xC0;
	else
		x = 0x81;
	vga->crt[0x50] |= x;

	vga->crt[0x51] = (vga->crt[0x51] & 0xC3)|((vga->crt[0x13]>>4) & 0x30);
	vga->crt[0x53] &= ~0x10;

	/*
	 * Set up linear aperture. For the moment it's 64K at 0xA0000.
	 * The real base address will be assigned before load is called.
	 */
	vga->crt[0x58] = 0x88;
	if(ctlr->flag & Uenhanced){
		vga->crt[0x58] |= 0x10;
		if(vga->linear && (ctlr->flag & Hlinear))
			ctlr->flag |= Ulinear;
		if(vga->vmz <= 1024*1024)
			vga->vma = 1024*1024;
		else if(vga->vmz <= 2*1024*1024)
			vga->vma = 2*1024*1024;
		else
			vga->vma = 8*1024*1024;
	}
	vga->crt[0x59] = 0x00;
	vga->crt[0x5A] = 0x0A;

	vga->crt[0x5D] &= 0x80;
	if(vga->crt[0x00] & 0x100)
		vga->crt[0x5D] |= 0x01;
	if(vga->crt[0x01] & 0x100)
		vga->crt[0x5D] |= 0x02;
	if(vga->crt[0x02] & 0x100)
		vga->crt[0x5D] |= 0x04;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x5D] |= 0x10;
	if(vga->crt[0x3B] & 0x100)
		vga->crt[0x5D] |= 0x40;

	vga->crt[0x5E] = 0x40;
	if(vga->crt[0x06] & 0x400)
		vga->crt[0x5E] |= 0x01;
	if(vga->crt[0x12] & 0x400)
		vga->crt[0x5E] |= 0x02;
	if(vga->crt[0x15] & 0x400)
		vga->crt[0x5E] |= 0x04;
	if(vga->crt[0x10] & 0x400)
		vga->crt[0x5E] |= 0x10;

	ctlr->type = s3generic.name;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ulong l;

	trace("%s->load->s3generic\n", ctlr->name);

	vgaxo(Crtx, 0x31, vga->crt[0x31]);
	vgaxo(Crtx, 0x32, vga->crt[0x32]);
	vgaxo(Crtx, 0x33, vga->crt[0x33]);
	vgaxo(Crtx, 0x34, vga->crt[0x34]);
	vgaxo(Crtx, 0x35, vga->crt[0x35]);
	vgaxo(Crtx, 0x3A, vga->crt[0x3A]);
	vgaxo(Crtx, 0x3B, vga->crt[0x3B]);
	vgaxo(Crtx, 0x3C, vga->crt[0x3C]);

	vgaxo(Crtx, 0x40, vga->crt[0x40]|0x01);
	vgaxo(Crtx, 0x42, vga->crt[0x42]);
	vgaxo(Crtx, 0x43, vga->crt[0x43]);
	vgaxo(Crtx, 0x45, vga->crt[0x45]);

	vgaxo(Crtx, 0x50, vga->crt[0x50]);
	vgaxo(Crtx, 0x51, vga->crt[0x51]);
	vgaxo(Crtx, 0x53, vga->crt[0x53]);
	vgaxo(Crtx, 0x54, vga->crt[0x54]);
	vgaxo(Crtx, 0x55, vga->crt[0x55]);

	if(ctlr->flag & Ulinear){
		l = vga->vmb>>16;
		vga->crt[0x59] = (l>>8) & 0xFF;
		vga->crt[0x5A] = l & 0xFF;
		if(vga->vmz <= 1024*1024)
			vga->crt[0x58] |= 0x01;
		else if(vga->vmz <= 2*1024*1024)
			vga->crt[0x58] |= 0x02;
		else
			vga->crt[0x58] |= 0x03;
	}
	vgaxo(Crtx, 0x59, vga->crt[0x59]);
	vgaxo(Crtx, 0x5A, vga->crt[0x5A]);
	vgaxo(Crtx, 0x58, vga->crt[0x58]);

	vgaxo(Crtx, 0x5D, vga->crt[0x5D]);
	vgaxo(Crtx, 0x5E, vga->crt[0x5E]);

	vgaxo(Crtx, 0x6A, vga->crt[0x6A]);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i, id, interlace, mul, div;
	char *name;
	ushort shb, vrs, x;

	name = ctlr->name;

	printitem(name, "Crt30");
	for(i = 0x30; i < 0x3E; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt40");
	for(i = 0x40; i < 0x50; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt50");
	for(i = 0x50; i < 0x60; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt60");
	for(i = 0x60; i < 0x70; i++)
		printreg(vga->crt[i]);

	/*
	 * Try to disassemble the snarfed values into
	 * understandable numbers.
	 * Only do this if we weren't called after Finit.
	 */
	if(ctlr->flag & Finit)
		return;


	/*
	 * If hde <= 400, assume this is a 928 or Vision964
	 * and the horizontal values have been divided by 4.
	 *
	 * if ViRGE/[DG]X and 15 or 16bpp, horizontal values have
	 * been multiplied by 2.
	 */
	mul = 1;
	div = 1;

	if(strcmp(name, "virge") == 0){
		id = (vga->crt[0x2D]<<8)|vga->crt[0x2E];
		/* S3 ViRGE/[DG]X */
		if(id==0x8A01 && ((vga->crt[0x67] & 0x30) || (vga->crt[0x67] & 0x50))){
			mul = 1;
			div = 2;
		}
	}

	x = vga->crt[0x01];
	if(vga->crt[0x5D] & 0x02)
		x |= 0x100;
	x = (x+1)<<3;

	if(x <= 400){
		mul = 4;
		div = 1;
	}

	x = (x * mul) / div;
	printitem(name, "hde");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	shb = vga->crt[0x02];
	if(vga->crt[0x5D] & 0x04)
		shb |= 0x100;
	shb = (shb+1)<<3;
	shb = (shb * mul) / div;
	printitem(name, "shb");
	printreg(shb);
	Bprint(&stdout, "%6ud", shb);

	x = vga->crt[0x03] & 0x1F;
	if(vga->crt[0x05] & 0x80)
		x |= 0x20;
	x = (x * mul) / div;
	x = shb|x;					/* ???? */
	if(vga->crt[0x5D] & 0x08)
		x += 64;
	printitem(name, "ehb");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	x = vga->crt[0x00];
	if(vga->crt[0x5D] & 0x01)
		x |= 0x100;
	x = (x+5)<<3;
	x = (x * mul) / div;
	printitem(name, "ht");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	interlace = vga->crt[0x42] & 0x20;
	x = vga->crt[0x12];
	if(vga->crt[0x07] & 0x02)
		x |= 0x100;
	if(vga->crt[0x07] & 0x40)
		x |= 0x200;
	if(vga->crt[0x5E] & 0x02)
		x |= 0x400;
	x += 1;
	if(interlace)
		x *= 2;
	printitem(name, "vde");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	vrs = vga->crt[0x10];
	if(vga->crt[0x07] & 0x04)
		vrs |= 0x100;
	if(vga->crt[0x07] & 0x80)
		vrs |= 0x200;
	if(vga->crt[0x5E] & 0x10)
		vrs |= 0x400;
	if(interlace)
		vrs *= 2;
	printitem(name, "vrs");
	printreg(vrs);
	Bprint(&stdout, "%6ud", vrs);

	if(interlace)
		vrs /= 2;
	x = (vrs & ~0x0F)|(vga->crt[0x11] & 0x0F);
	if(interlace)
		x *= 2;
	printitem(name, "vre");
	printreg(x);
	Bprint(&stdout, "%6ud", x);

	x = vga->crt[0x06];
	if(vga->crt[0x07] & 0x01)
		x |= 0x100;
	if(vga->crt[0x07] & 0x20)
		x |= 0x200;
	if(vga->crt[0x5E] & 0x01)
		x |= 0x400;
	x += 2;
	if(interlace)
		x *= 2;
	printitem(name, "vt");
	printreg(x);
	Bprint(&stdout, "%6ud", x);
}

Ctlr s3generic = {
	"s3",				/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
