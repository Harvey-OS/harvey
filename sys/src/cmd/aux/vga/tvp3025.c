#include <u.h>
#include <libc.h>

#include "vga.h"

/*
 * Tvp3025 Viewpoint Video Interface Pallette.
 * Assumes hooked up to an S3 Vision964.
 * The #9GXE64pro uses bit 5 of Crt5C as RS4,
 * giving access to the Bt485 emulation mode registers.
 */
static void
options(Vga *vga, Ctlr *ctlr)
{
	(*tvp3020.options)(vga, ctlr);
}

static void
init(Vga *vga, Ctlr *ctlr)
{
	(*tvp3020.init)(vga, ctlr);
}

static void
load(Vga *vga, Ctlr *ctlr)
{
	uchar crt5c, x;

	crt5c = vgaxi(Crtx, 0x5C) & ~0x20;
	vgaxo(Crtx, 0x5C, crt5c);
	x = tvp3020xi(0x06) & ~0x80;
	tvp3020xo(0x06, x);
	(*tvp3020.load)(vga, ctlr);
}

static void
dump(Vga *vga, Ctlr *ctlr)
{
	uchar crt5c;
	int i;

	crt5c = vgaxi(Crtx, 0x5C);
	vgaxo(Crtx, 0x5C, crt5c & ~0x20);
	(*tvp3020.dump)(vga, ctlr);

	printitem(ctlr->name, "PCLK");
	for(i = 0; i < 4; i++){
		tvp3020xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(tvp3020xi(0x2D));
	}
	printitem(ctlr->name, "MCLK");
	for(i = 0; i < 4; i++){
		tvp3020xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(tvp3020xi(0x2E));
	}
	printitem(ctlr->name, "RCLK");
	for(i = 0; i < 4; i++){
		tvp3020xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(tvp3020xi(0x2F));
	}

	vgaxo(Crtx, 0x5C, crt5c);
}

Ctlr tvp3025 = {
	"tvp3025",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
