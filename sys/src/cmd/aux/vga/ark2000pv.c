#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ARK Logic ARK2000PV GUI accelerator.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;

	/*
	 * Unlock access to the extended I/O registers.
	 */
	vgaxo(Seqx, 0x1D, 0x01);

	for(i = 0x10; i < 0x2E; i++)
		vga->sequencer[i] = vgaxi(Seqx, i);
	for(i = 0x40; i < 0x47; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	vga->crt[0x50] = vgaxi(Crtx, 0x50);

	/*
	 * A hidey-hole for the coprocessor status register.
	 */
	vga->crt[0xFF] = inportb(0x3CB);

	/*
	 * Memory size.
	 */
	switch((vga->sequencer[0x10]>>6) & 0x03){

	case 0x00:
		vga->vma = vga->vmz = 1*1024*1024;
		break;

	case 0x01:
		vga->vma = vga->vmz = 2*1024*1024;
		break;

	case 0x02:
		vga->vma = vga->vmz = 4*1024*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Hpclk2x8|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	ulong x;

	mode = vga->mode;

	vga->crt[0x46] = 0x00;
	if(ctlr->flag & Upclk2x8){
		vga->crt[0x00] = ((mode->ht/2)>>3)-5;
		vga->crt[0x01] = ((mode->x/2)>>3)-1;
		vga->crt[0x02] = ((mode->shb/2)>>3)-1;
	
		x = (mode->ehb/2)>>3;
		vga->crt[0x03] = 0x80|(x & 0x1F);
		vga->crt[0x04] = (mode->shs/2)>>3;
		vga->crt[0x05] = ((mode->ehs/2)>>3) & 0x1F;
		if(x & 0x20)
			vga->crt[0x05] |= 0x80;
		vga->crt[0x13] = mode->x/8;

		vga->crt[0x46] |= 0x04;
	}

	/*
	 * Overflow bits.
	 */
	vga->crt[0x40] = 0x00;
	if(vga->crt[0x18] & 0x400)
		vga->crt[0x40] |= 0x08;
	if(vga->crt[0x10] & 0x400)
		vga->crt[0x40] |= 0x10;
	if(vga->crt[0x15] & 0x400)
		vga->crt[0x40] |= 0x20;
	if(vga->crt[0x12] & 0x400)
		vga->crt[0x40] |= 0x40;
	if(vga->crt[0x06] & 0x400)
		vga->crt[0x40] |= 0x80;

	vga->crt[0x41] = 0x00;
	if(vga->crt[0x13] & 0x100)
		vga->crt[0x41] |= 0x08;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x41] |= 0x10;
	if(vga->crt[0x02] & 0x100)
		vga->crt[0x41] |= 0x20;
	if(vga->crt[0x01] & 0x100)
		vga->crt[0x41] |= 0x40;
	if(vga->crt[0x00] & 0x100)
		vga->crt[0x41] |= 0x80;

	/*
	 * Interlace.
	 */
	vga->crt[0x42] = 0x00;
	vga->crt[0x44] = 0x00;
	if(mode->interlace){
		vga->crt[0x42] = vga->crt[0]/2;
		vga->crt[0x44] |= 0x04;
	}
	vga->crt[0x45] = 0x00;

	/*
	 * Memory configuration:
	 * enable linear|coprocessor|VESA modes;
	 * set aperture to 64Kb, 0xA0000;
	 * set aperture index 0.
	 * Bugs: 1024x768x1 doesn't work (aperture not set correctly?);
	 *	 hwgc doesn't work in 1-bit modes (hardware?).
	 */
	vga->sequencer[0x10] &= ~0x3F;
	vga->sequencer[0x11] &= ~0x0F;
	switch(mode->z){

	case 1:
		vga->sequencer[0x10] |= 0x03;
		vga->sequencer[0x11] |= 0x01;
		cflag = 1;
		break;

	case 8:
		vga->sequencer[0x10] |= 0x0F;
		vga->sequencer[0x11] |= 0x06;
		if(vga->linear && (ctlr->flag & Hlinear) && vga->vmz)
			ctlr->flag |= Ulinear;
		break;

	default:
		error("depth %d not supported\n", mode->z);
	}
	vga->sequencer[0x12] &= ~0x03;
	vga->sequencer[0x13] = 0x0A;
	vga->sequencer[0x14] = 0x00;
	vga->sequencer[0x15] = 0x00;
	vga->sequencer[0x16] = 0x00;

	/*
	 * Frame Buffer Pitch and FIFO control.
	 * Set the FIFO to 32 deep and refill trigger when
	 * 6 slots empty.
	 */
	vga->sequencer[0x17] = 0x00;
	vga->sequencer[0x18] = 0x13;
	if(mode->x <= 640)
		vga->sequencer[0x17] = 0x00;
	else if(mode->x <= 800)
		vga->sequencer[0x17] |= 0x01;
	else if(mode->x <= 1024)
		vga->sequencer[0x17] |= 0x02;
	else if(mode->x <= 1280)
		vga->sequencer[0x17] |= 0x04;
	else if(mode->x <= 1600)
		vga->sequencer[0x17] |= 0x05;
	else if(mode->x <= 2048)
		vga->sequencer[0x17] |= 0x06;

	/*
	 * Clock select.
	 */
	vga->misc &= ~0x0C;
	vga->misc |= (vga->i[0] & 0x03)<<2;
	vga->sequencer[0x11] &= ~0xC0;
	vga->sequencer[0x11] |= (vga->i[0] & 0x0C)<<4;

	vga->attribute[0x11] = 0x00;	/* color map entry for black */

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ulong l;

	/*
	 * Ensure there are no glitches when selecting a new
	 * clock frequency.
	 * The sequencer toggle seems to matter on the Hercules
	 * Stingray 64/Video at 1280x1024x8. Without it the screen
	 * is fuzzy; a second load clears it up and there's no
	 * difference between the two register sets. A mystery.
	 */
	vgao(MiscW, vga->misc & ~0x0C);
	vgaxo(Seqx, 0x11, vga->sequencer[0x11]);
	vgao(MiscW, vga->misc);
	if(vga->ramdac && strncmp(vga->ramdac->name, "w30c516", 7) == 0){
		sequencer(vga, 1);
		sleep(500);
		sequencer(vga, 0);
	}

	if(ctlr->flag & Ulinear){
		vga->sequencer[0x10] |= 0x10;
		if(vga->vmz <= 1024*1024)
			vga->sequencer[0x12] |= 0x01;
		else if(vga->vmz <= 2*1024*1024)
			vga->sequencer[0x12] |= 0x02;
		else
			vga->sequencer[0x12] |= 0x03;
		l = vga->vmb>>16;
		vga->sequencer[0x13] = l & 0xFF;
		vga->sequencer[0x14] = (l>>8) & 0xFF;
	}

	vgaxo(Seqx, 0x10, vga->sequencer[0x10]);
	vgaxo(Seqx, 0x12, vga->sequencer[0x12]);
	vgaxo(Seqx, 0x13, vga->sequencer[0x13]);
	vgaxo(Seqx, 0x14, vga->sequencer[0x14]);
	vgaxo(Seqx, 0x15, vga->sequencer[0x15]);
	vgaxo(Seqx, 0x16, vga->sequencer[0x16]);
	vgaxo(Seqx, 0x17, vga->sequencer[0x17]);
	vgaxo(Seqx, 0x18, vga->sequencer[0x18]);

	vgaxo(Crtx, 0x40, vga->crt[0x40]);
	vgaxo(Crtx, 0x41, vga->crt[0x41]);
	vgaxo(Crtx, 0x42, vga->crt[0x42]);
	vgaxo(Crtx, 0x44, vga->crt[0x44]);
	vgaxo(Crtx, 0x45, vga->crt[0x45]);
	vgaxo(Crtx, 0x46, vga->crt[0x46]);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "Seq10");
	for(i = 0x10; i < 0x2E; i++)
		printreg(vga->sequencer[i]);
	printitem(ctlr->name, "Crt40");
	for(i = 0x40; i < 0x47; i++)
		printreg(vga->crt[i]);
	printitem(ctlr->name, "Crt50");
	printreg(vga->crt[0x50]);
	printitem(ctlr->name, "Cop status");
	printreg(vga->crt[0xFF]);
}

Ctlr ark2000pv = {
	"ark2000pv",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr ark2000pvhwgc = {
	"ark2000pvhwgc",		/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
