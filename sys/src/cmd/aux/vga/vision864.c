/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
options(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Hpclk2x8|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	uint32_t x;
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
	if((val = dbattr(vga->attr, "delaybl")) != nil)
		vga->crt[0x6D] |= strtoul(val, 0, 0) & 0x07;
	else
		vga->crt[0x6D] |= 2;
	if((val = dbattr(vga->attr, "delaysc")) != nil)
		vga->crt[0x6D] |= (strtoul(val, 0, 0) & 0x07)<<4;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uint16_t advfunc;

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
