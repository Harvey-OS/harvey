#include <u.h>
#include <libc.h>
#include <bio.h>

#include "vga.h"

/*
 * Trident Cyber938x.
 */
typedef struct {
	uchar	old[3];

	int	x;
	int	y;
} Cyber938x;

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Cyber938x *cyber;

	if(vga->private == nil)
		vga->private = alloc(sizeof(Cyber938x));
	cyber = vga->private;

	/*
	 * Switch to old mode (write to Seq0B), read the old
	 * mode registers (Seq0[CDE]) then switch to new mode
	 * (read Seq0B).
	 */
	vgaxo(Seqx, 0x0B, 0);
	cyber->old[0] = vgaxi(Seqx, 0x0C);
	cyber->old[1] = vgaxi(Seqx, 0x0D);
	cyber->old[2] = vgaxi(Seqx, 0x0E);
	vga->sequencer[0x0B] = vgaxi(Seqx, 0x0B);

	for(i = 0x08; i < 0x10; i++){
		if(i == 0x0B)
			continue;
		vga->sequencer[i] = vgaxi(Seqx, i);
	}
	for(i = 0x19; i < 0x60; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	for(i = 0x09; i < 0x60; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	switch(vga->crt[0x1F] & 0x07){
	case 1:
	case 5:
		vga->vmz = 4*1024*1024;
		break;

	case 2:
	case 6:
		vga->vmz = 768*1024;
		break;

	case 7:
		vga->vmz = 2*1024*1024;
		break;

	case 3:
	default:
		vga->vmz = 1024*1024;
		break;
	}
	vga->vma = 4*1024*1024;

	switch((vga->graphics[0x52]>>4) & 0x03){
	case 0:
		cyber->x = 1280;
		cyber->y = 1024;
		break;

	case 1:
		cyber->x = 640;
		cyber->y = 480;
		break;

	case 2:
		cyber->x = 1024;
		cyber->y = 768;
		break;

	case 3:
		cyber->x = 800;
		cyber->y = 600;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	USED(vga);
	ctlr->flag |= Hlinear|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	vga->crt[0x1E] = 0x80;
	vga->crt[0x2A] |= 0x40;
	vga->crt[0x38] = 0x00;

	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	vga->graphics[0x31] &= 0x8F;
	if(vga->mode->y > 768)
		vga->graphics[0x31] |= 0x30;
	else if(vga->mode->y > 600)
		vga->graphics[0x31] |= 0x20;
	else if(vga->mode->y > 480)
		vga->graphics[0x31] |= 0x10;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);


	switch(vga->sequencer[0x09]){

	default:
		error("unknown Cyber revision 0x%uX\n", vga->sequencer[0x09]);
		break;

	case 0x42:				/* Cyber9382 */
		/*
		 * Try a little magic for DSTN displays.
		 * May work on other chips too, who knows.
		 */
		if(!(vga->graphics[0x42] & 0x80)	/* !TFT */
		&& (vga->graphics[0x43] & 0x20)){	/* DSTN */
			if(vga->mode->x == 640)
				vga->graphics[0x30] |= 0x81;
			else
				vga->graphics[0x30] &= ~0x81;
		}
		/*FALLTHROUGH*/
	case 0x33:				/* Cyber9385 */
	case 0x35:				/* Cyber9385 */
		vga->graphics[0x0F] |= 0x07;
		break;

	case 0x4A:				/* Cyber9388 */
		vga->crt[0x2F] = 0x3F;
		vga->graphics[0x0F] |= 0x17;
		if(vga->mode->x == 1024)
			vga->graphics[0x30] = 0x18;
		break;
	}
	vga->graphics[0x52] &= ~0x01;
	vga->graphics[0x53] &= ~0x01;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	outportw(0x3C4, 0x000B);
	outportb(0x3C4, 0x0B);
	inportb(0x3C5);

	outportw(0x3C4, (0xC2 << 8) | 0x0E);
	vgaxo(Grx, 0x52, vga->graphics[0x52]);
	vgaxo(Grx, 0x53, vga->graphics[0x53]);
	vgaxo(Grx, 0x30, vga->graphics[0x30]);
	vgaxo(Grx, 0x31, vga->graphics[0x31]);

	vgaxo(Crtx, 0x1E, vga->crt[0x1E]);
	if(ctlr->flag & Ulinear)
		vga->crt[0x21] |= 0x20;
	vgaxo(Crtx, 0x21, vga->crt[0x21]);
	vgaxo(Crtx, 0x2A, vga->crt[0x2A]);
	vgaxo(Crtx, 0x2F, vga->crt[0x2F]);
	vgaxo(Crtx, 0x38, vga->crt[0x38]);

	vgaxo(Grx, 0x0F, vga->graphics[0x0F]);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;
	Cyber938x *cyber;

	name = ctlr->name;
	cyber = vga->private;
	printitem(name, "Seq08");
	for(i = 0x08; i < 0x10; i++)
		printreg(vga->sequencer[i]);
	printitem(name, "Old Seq0C");
	for(i = 0; i < 3; i++)
		printreg(cyber->old[i]);
	printitem(name, "Crt19");
	for(i = 0x19; i < 0x20; i++)
		printreg(vga->crt[i]);
	printitem(name, "Crt20");
	for(i = 0x20; i < 0x60; i++)
		printreg(vga->crt[i]);
	printitem(name, "Gr09");
	for(i = 0x09; i < 0x10; i++)
		printreg(vga->graphics[i]);
	printitem(name, "Gr10");
	for(i = 0x10; i < 0x60; i++)
		printreg(vga->graphics[i]);

	printitem(name, "mclk");
	printreg(inportb(0x43C6));
	printreg(inportb(0x43C7));
	printitem(name, "vclk");
	printreg(inportb(0x43C8));
	printreg(inportb(0x43C9));

	printitem(name, "lcd size");
	Bprint(&stdout, "%9ud %8ud\n", cyber->x, cyber->y);
}

Ctlr cyber938x = {
	"cyber938x",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr cyber938xhwgc = {
	"cyber938xhwgc",		/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
