#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * S3 86C80[15] GUI Accelerator.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	s3generic.snarf(vga, ctlr);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong x;

	s3generic.init(vga, ctlr);
	vga->crt[0x3B] = vga->crt[0]-5;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	/*
	 * Display memory access control.
	 * Calculation of the M-parameter (Crt54) is
	 * memory-system and dot-clock dependent, the
	 * values below are guesses from dumping
	 * registers.
	 */
	vga->crt[0x60] = 0xFF;
	x = (vga->mode->x)/4;
	vga->crt[0x61] = 0x80|((x>>8) & 0x07);
	vga->crt[0x62] = (x & 0xFF);

	if(vga->mode->x <= 800)
		vga->crt[0x54] = 0x88;
	else if(vga->mode->x <= 1024)
		vga->crt[0x54] = 0xF8;
	else
		vga->crt[0x54] = 0x40;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ushort advfunc;

	s3generic.load(vga, ctlr);
	vgaxo(Crtx, 0x60, vga->crt[0x60]);
	vgaxo(Crtx, 0x61, vga->crt[0x61]);
	vgaxo(Crtx, 0x62, vga->crt[0x62]);

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

Ctlr s3801 = {
	"s3801",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr s3805 = {
	"s3805",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
