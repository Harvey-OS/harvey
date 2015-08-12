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
 * Tvp3025 Viewpoint Video Interface Pallette.
 * Assumes hooked up to an S3 Vision964.
 * The #9GXE64pro uses bit 5 of Crt5C as RS4,
 * giving access to the Bt485 emulation mode registers.
 */
static void
options(Vga *vga, Ctlr *ctlr)
{
	tvp3020.options(vga, ctlr);
}

static void
init(Vga *vga, Ctlr *ctlr)
{
	/*
	 * Although the Tvp3025 has a higher default
	 * speed-grade (135MHz), just use the 3020 code.
	 */
	tvp3020.init(vga, ctlr);
}

static void
load(Vga *vga, Ctlr *ctlr)
{
	uint8_t crt5c, x;

	crt5c = vgaxi(Crtx, 0x5C) & ~0x20;
	vgaxo(Crtx, 0x5C, crt5c);
	x = tvp3020xi(0x06) & ~0x80;
	tvp3020xo(0x06, x);
	tvp3020xo(0x0E, 0x00);

	(tvp3020.load)(vga, ctlr);
	if(ctlr->flag & Uenhanced)
		tvp3020xo(0x29, 0x01);

	ctlr->flag |= Fload;
}

static uint32_t
dumpclock(uint32_t d, uint32_t n, uint32_t p)
{
	uint32_t f;

	f = RefFreq * ((n + 2) * 8);
	f /= (d + 2);
	f >>= p;

	return f;
}

static void
dump(Vga *vga, Ctlr *ctlr)
{
	uint8_t crt5c;
	int i;
	uint32_t clock[4];

	crt5c = vgaxi(Crtx, 0x5C);
	vgaxo(Crtx, 0x5C, crt5c & ~0x20);
	tvp3020.dump(vga, ctlr);

	printitem(ctlr->name, "PCLK");
	for(i = 0; i < 4; i++) {
		tvp3020xo(0x2C, (i << 4) | (i << 2) | i);
		printreg(clock[i] = tvp3020xi(0x2D));
	}
	Bprint(&stdout, "%23ld\n", dumpclock(clock[0], clock[1], clock[2] & 0x07));

	printitem(ctlr->name, "MCLK");
	for(i = 0; i < 4; i++) {
		tvp3020xo(0x2C, (i << 4) | (i << 2) | i);
		printreg(clock[i] = tvp3020xi(0x2E));
	}
	Bprint(&stdout, "%23ld\n", dumpclock(clock[0], clock[1], clock[2] & 0x07));

	printitem(ctlr->name, "RCLK");
	for(i = 0; i < 4; i++) {
		tvp3020xo(0x2C, (i << 4) | (i << 2) | i);
		printreg(clock[i] = tvp3020xi(0x2F));
	}
	Bprint(&stdout, "%23ld\n", dumpclock(clock[0], clock[1], clock[2] & 0x07));

	vgaxo(Crtx, 0x5C, crt5c);
}

Ctlr tvp3025 = {
    "tvp3025", /* name */
    0,	 /* snarf */
    options,   /* options */
    init,      /* init */
    load,      /* load */
    dump,      /* dump */
};
