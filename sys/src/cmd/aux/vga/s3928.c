#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * S3 86C928 GUI Accelerator.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	s3generic.snarf(vga, ctlr);
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

	/*
	 * s3generic.init() will perform the same test.
	 * We need to do it here too so that the Crt registers
	 * will be correct when s3generic.init() calculates
	 * the overflow bits.
	 */
	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	mode = vga->mode;
	if((ctlr->flag & Henhanced) && mode->x >= 1024 && mode->z == 8){
		resyncinit(vga, ctlr, Uenhanced, 0);
		vga->crt[0x00] = ((mode->ht/4)>>3)-5;
		vga->crt[0x01] = ((mode->x/4)>>3)-1;
		vga->crt[0x02] = ((mode->shb/4)>>3)-1;
	
		x = (mode->ehb/4)>>3;
		vga->crt[0x03] = 0x80|(x & 0x1F);
		vga->crt[0x04] = (mode->shs/4)>>3;
		vga->crt[0x05] = ((mode->ehs/4)>>3) & 0x1F;
		if(x & 0x20)
			vga->crt[0x05] |= 0x80;

		vga->crt[0x13] = mode->x/8;
		if(mode->z == 1)
			vga->crt[0x13] /= 8;
	}
	s3generic.init(vga, ctlr);
	vga->crt[0x3B] = (vga->crt[0]+vga->crt[4]+1)/2;

	/*
	 * Set up write-posting and read-ahead-cache.
	 */
	vga->crt[0x54] = 0xA7;
	if(ctlr->flag & Uenhanced){
		vga->crt[0x58] |= 0x04;
		vga->crt[0x40] |= 0x08;
	}
	else{
		vga->crt[0x58] &= ~0x04;
		vga->crt[0x40] &= ~0x08;
	}

	/*
	 * Set up parallel VRAM and the external
	 * SID enable on the 86C928
	 */
	vga->crt[0x53] &= ~0x20;			/* parallel VRAM */
	vga->crt[0x55] &= ~0x48;			/* external SID */
	vga->crt[0x65] &= ~0x60;			/* 2 86C928 E-step chip bugs */
	if(ctlr->flag & Uenhanced){
		if(vga->ramdac->flag & Hpvram)
			vga->crt[0x53] |= 0x20;
		if(vga->ramdac->flag & Hextsid)
			vga->crt[0x55] |= 0x48;
		vga->crt[0x65] |= 0x60;
	}
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ushort advfunc;

	(*s3generic.load)(vga, ctlr);
	vgaxo(Crtx, 0x65, vga->crt[0x65]);

	advfunc = 0x0000;
	if(ctlr->flag & Uenhanced){
		if(vga->mode->x == 1024 || vga->mode->x == 800)
			advfunc = 0x0057;
		else
			advfunc = 0x0053;
	}
	outportw(0x4AE8, advfunc);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	(*s3generic.dump)(vga, ctlr);
}

Ctlr s3928 = {
	"s3928",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
