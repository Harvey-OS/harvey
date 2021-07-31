#include <u.h>
#include <libc.h>

#include "vga.h"

/*
 * Generic S3 GUI Accelerator.
 */
static void
unlock(void)
{
	/*
	 * 0xA5 ensures Crt36 and Crt37 are also unlocked
	 * (0xA0 unlocks everything else).
	 */
	vgaxo(Crtx, 0x38, 0x48);
	vgaxo(Crtx, 0x39, 0xA5);
}

static void
snarf(Vga *vga, Ctlr *ctlr)
{
	int i;

	verbose("%s->snarf->s3generic\n", ctlr->name);

	unlock();
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
		vga->vmb = 4*1024*1024;
		break;

	case 0x02:
		vga->vmb = 3*1024*1024;
		break;

	case 0x04:
		vga->vmb = 2*1024*1024;
		break;

	case 0x06:
		vga->vmb = 1*1024*1024;
		break;

	case 0x07:
		vga->vmb = 512*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
init(Vga *vga, Ctlr *ctlr)
{
	Mode *mode;
	ulong x;

	verbose("%s->init->s3generic\n", ctlr->name);
	mode = vga->mode;

	/*
	 * Do we need to use enhanced mode?
	 */
	if((ctlr->flag & (Uenhanced|Henhanced)) == Henhanced){
		if(mode->x >= 1024 && mode->z == 8)
			resyncinit(vga, ctlr, Uenhanced, 0);
		else
			resyncinit(vga, ctlr, 0, Uenhanced|Henhanced);
	}

	vga->crt[0x31] = 0x85;
	vga->crt[0x32] &= ~0x40;

	vga->crt[0x31] |= 0x08;
	vga->crt[0x32] |= 0x40;

	vga->crt[0x33] |= 0x20;
	if(mode->z == 8)
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

	vga->crt[0x50] &= 0x3E;
	x = 0x00;
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
	else if(mode->x <= 1600)
		x = 0x81;
	vga->crt[0x50] |= x;

	vga->crt[0x51] = (vga->crt[0x51] & 0xC3)|((vga->crt[0x13]>>4) & 0x30);
	vga->crt[0x53] &= ~0x10;

	/*
	 * Set up linear aperture.
	 * For the moment it's 64K at 0xA0000.
	 */
	vga->crt[0x58] = 0x88;
	if(ctlr->flag & Uenhanced)
		vga->crt[0x58] |= 0x10;
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
load(Vga *vga, Ctlr *ctlr)
{
	verbose("%s->load->s3generic\n", ctlr->name);

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
	vgaxo(Crtx, 0x59, vga->crt[0x59]);
	vgaxo(Crtx, 0x5A, vga->crt[0x5A]);
	vgaxo(Crtx, 0x58, vga->crt[0x58]);
	vgaxo(Crtx, 0x5D, vga->crt[0x5D]);
	vgaxo(Crtx, 0x5E, vga->crt[0x5E]);

	ctlr->flag |= Fload;
}

static void
dump(Vga *vga, Ctlr *ctlr)
{
	int i, interlace, mul;
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
	 * and the horozontal values have been divided by 4.
	 */
	mul = 1;
	x = vga->crt[0x01];
	if(vga->crt[0x5D] & 0x02)
		x |= 0x100;
	x = (x+1)<<3;
	if(x <= 400)
		mul = 4;
	x *= mul;
	printitem(name, "hde");
	printreg(x);
	print("%6hud", x);

	shb = vga->crt[0x02];
	if(vga->crt[0x5D] & 0x04)
		shb |= 0x100;
	shb = (shb+1)<<3;
	shb *= mul;
	printitem(name, "shb");
	printreg(shb);
	print("%6hud", shb);

	x = vga->crt[0x03] & 0x1F;
	if(vga->crt[0x05] & 0x80)
		x |= 0x20;
	x *= mul;
	x = shb|x;					/* ???? */
	if(vga->crt[0x5D] & 0x08)
		x += 64;
	printitem(name, "ehb");
	printreg(x);
	print("%6hud", x);

	x = vga->crt[0x00];
	if(vga->crt[0x5D] & 0x01)
		x |= 0x100;
	x = (x+5)<<3;
	x *= mul;
	printitem(name, "ht");
	printreg(x);
	print("%6hud", x);

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
	print("%6hud", x);

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
	print("%6hud", vrs);

	if(interlace)
		vrs /= 2;
	x = (vrs & ~0x0F)|(vga->crt[0x11] & 0x0F);
	if(interlace)
		x *= 2;
	printitem(name, "vre");
	printreg(x);
	print("%6hud", x);

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
	print("%6hud", x);
}

Ctlr s3generic = {
	"s3",				/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
