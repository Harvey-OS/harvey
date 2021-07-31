#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#include <libg.h>
#include "screen.h"
#include "vga.h"

static void
mach32page(int page)
{
	uchar ae, p;


	p = (page & 0x0F)<<1;
	p |= (page & 0x07)<<5;
	outs(0x1CE, (p<<8)|0xB2);

	outb(0x1CE, 0xAE);
	ae = inb(0x1CE+1);

	p = (page>>4) & 0x03;
	p |= p<<2;
	p |= ae & 0xF0;
	outs(0x1CE, (p<<8)|0xAE);
}

static Vgac mach32 = {
	"mach32",
	mach32page,

	0,
};

void
vgamach32link(void)
{
	addvgaclink(&mach32);
}
