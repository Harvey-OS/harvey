#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * S3 Vision864 GUI Accelerator.
 * Pretty much the same as the 86C80[15].
 * First pass, needs tuning.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	s3generic.snarf(vga, ctlr);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Hpclk2x8|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong x;
	char *val;

	s3generic.init(vga, ctlr);
	vga->crt[0x3B] = vga->crt[0]-5;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	/*
	 * VL-bus crap.
	 */
	if((vga->crt[0x36] & 0x03) == 0x01){
		vga->crt[0x40] |= 0x08;
		vga->crt[0x58] &= ~0x88;
	}

	/*
	 * Display memory access control.
	 * Calculation of the M-parameter (Crt54) is
	 * memory-system and dot-clock dependent, the
	 * values below are guesses from dumping
	 * registers.
	 */
	vga->crt[0x60] = 0xFF;
	x = vga->mode->x/8;
	vga->crt[0x61] = 0x80|((x>>8) & 0x07);
	vga->crt[0x62] = (x & 0xFF);
	if(vga->mode->x <= 800)
		vga->crt[0x54] = 0x88;
	else if(vga->mode->x <= 1024)
		vga->crt[0x54] = 0xF8;
	else
		vga->crt[0x54] = 0x40;

	vga->crt[0x67] &= ~0xF0;
	if(ctlr->flag & Upclk2x8)
		vga->crt[0x67] |= 0x10;

	vga->crt[0x69] = 0x00;
	vga->crt[0x6A] = 0x00;

	/*
	 * Blank adjust.
	 * This may not be correct for all monitors.
	 */
	vga->crt[0x6D] = 0x00;
	if(val = dbattr(vga->attr, "delaybl"))
		vga->crt[0x6D] |= strtoul(val, 0, 0) & 0x07;
	else
		vga->crt[0x6D] |= 2;
	if(val = dbattr(vga->attr, "delaysc"))
		vga->crt[0x6D] |= (strtoul(val, 0, 0) & 0x07)<<4;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ushort advfunc;

	s3generic.load(vga, ctlr);
	vgaxo(Crtx, 0x60, vga->crt[0x60]);
	vgaxo(Crtx, 0x61, vga->crt[0x61]);
	vgaxo(Crtx, 0x62, vga->crt[0x62]);
	vgaxo(Crtx, 0x67, vga->crt[0x67]);
	vgaxo(Crtx, 0x69, vga->crt[0x69]);
	vgaxo(Crtx, 0x6A, vga->crt[0x6A]);
	vgaxo(Crtx, 0x6D, vga->crt[0x6D]);

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
	s3generic.dump(vga, ctlr);
}

Ctlr vision864 = {
	"vision864",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
