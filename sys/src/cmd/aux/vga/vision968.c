#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * S3 Vision968 GUI Accelerator.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	s3generic.snarf(vga, ctlr);

	vga->sequencer[0x09] = vgaxi(Seqx, 0x09);
	vga->sequencer[0x0A] = vgaxi(Seqx, 0x0A);

	vga->crt[0x22] = vgaxi(Crtx, 0x22);
	vga->crt[0x24] = vgaxi(Crtx, 0x24);
	vga->crt[0x26] = vgaxi(Crtx, 0x26);
	vga->crt[0x2D] = vgaxi(Crtx, 0x2D);
	vga->crt[0x2E] = vgaxi(Crtx, 0x2E);
	vga->crt[0x2F] = vgaxi(Crtx, 0x2F);
}

static void
options(Vga*, Ctlr* ctlr)
{	
	ctlr->flag |= Hlinear|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	ulong x;
	int sid, dbl, bpp, divide;
	char *val;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	mode = vga->mode;
	if(vga->ramdac && (vga->ramdac->flag & Uclk2)){
		resyncinit(vga, ctlr, Uenhanced, 0);
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
	}
	else if(mode->z == 8)
		resyncinit(vga, ctlr, Uenhanced, 0);
	s3generic.init(vga, ctlr);
	/*
	if((ctlr->flag & Uenhanced) == 0)
		vga->crt[0x33] &= ~0x20;
	 */
	vga->crt[0x3B] = (vga->crt[0]+vga->crt[4]+1)/2;
	if(vga->crt[0x3B] & 0x100)
		vga->crt[0x5D] |= 0x40;
	vga->crt[0x55] = 0x00;

	vga->crt[0x40] &= ~0x10;
	vga->crt[0x53] &= ~0x20;
	vga->crt[0x58] &= ~0x48;
	vga->crt[0x65] = 0x00;
	vga->crt[0x66] &= ~0x07;
	vga->crt[0x67] = 0x00;
	vga->crt[0x6D] = 0x00;
	if(ctlr->flag & Uenhanced){
		if(vga->ramdac && (vga->ramdac->flag & Hextsid)){
			if(vga->ramdac->flag & Hsid32)
				sid = 32;
			else
				sid = 64;
			if(vga->ramdac->flag & Uclk2)
				dbl = 2;
			else
				dbl = 1;
			if(mode->z < 4)
				bpp = 4;
			else
				bpp = mode->z;
			divide = sid/(dbl*bpp);
			switch(divide){
			case 2:
				vga->crt[0x66] |= 0x01;
				break;
			case 4:
				vga->crt[0x66] |= 0x02;
				break;
			case 8:
				vga->crt[0x66] |= 0x03;
				break;
			case 16:
				vga->crt[0x66] |= 0x04;
				break;
			case 32:
				vga->crt[0x66] |= 0x05;
				break;
			}
		}

		vga->crt[0x40] |= 0x10;
		vga->crt[0x58] |= 0x40;

		/*
		 * The test against 1376 is necessary for the STB
		 * Velocity 64 Video, no idea if it's relevant to other
		 * cards.
		 * Although not mentioned in the databook, bit 4 of Crt67
		 * needs to be set on the STB Velocity 64 Video too. Gak.
		 */
		if(dbattr(vga->attr, "disa1sc") && mode->x <= 1376)
			vga->crt[0x65] |= 0x02;
		if(vga->ramdac && strncmp(vga->ramdac->name, "tvp3026", 7))
			vga->crt[0x67] |= 0x10;

		if(dbattr(vga->attr, "vclkphs"))
			vga->crt[0x67] |= 0x01;
		if(val = dbattr(vga->attr, "delaybl"))
			vga->crt[0x6D] |= strtoul(val, 0, 0) & 0x07;
		if(val = dbattr(vga->attr, "delaysc"))
			vga->crt[0x6D] |= (strtoul(val, 0, 0) & 0x07)<<4;
	}
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ushort advfunc;

	s3generic.load(vga, ctlr);
	vgaxo(Crtx, 0x65, vga->crt[0x65]);
	vgaxo(Crtx, 0x66, vga->crt[0x66]);
	vgaxo(Crtx, 0x67, vga->crt[0x67]);
	vgaxo(Crtx, 0x6D, vga->crt[0x6D]);

	advfunc = 0x0000;
	if(ctlr->flag & Uenhanced)
		advfunc = 0x0001;
	outportw(0x4AE8, advfunc);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	s3generic.dump(vga, ctlr);

	printitem(ctlr->name, "Seq09");
	printreg(vga->sequencer[0x09]);
	printreg(vga->sequencer[0x0A]);

	printitem(ctlr->name, "Crt22");
	printreg(vga->crt[0x22]);
	printitem(ctlr->name, "Crt24");
	printreg(vga->crt[0x24]);
	printitem(ctlr->name, "Crt26");
	printreg(vga->crt[0x26]);
	printitem(ctlr->name, "Crt2D");
	printreg(vga->crt[0x2D]);
	printreg(vga->crt[0x2E]);
	printreg(vga->crt[0x2F]);
}

Ctlr vision968 = {
	"vision968",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
