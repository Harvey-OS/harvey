#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

static void
init(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Finit;

	/*
	 * Use of the hwgc requires
	 *	a W32 chip,
	 *	8-bits,
	 *	not 2x8-bit mode.
	 */
	if(cflag)
		return;
	if(vga->ctlr == 0 || strncmp(vga->ctlr->name, "et4000-w32", 10))
		cflag = 1;
	if(vga->mode->z != 8 || (ctlr->flag & Upclk2x8))
		cflag = 1;
}

Ctlr et4000hwgc = {
	"et4000hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
