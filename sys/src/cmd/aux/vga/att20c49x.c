#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ATT20C490 and ATT20C49[12] True-Color CMOS RAMDACs.
 */
enum {
	Cr0		= 0x00,		/* Control register 0 */
};

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong pclk;
	char *p;

	/*
	 * Part comes in -100, -80, -65 and -55MHz speed-grades.
	 * Work out the part speed-grade from name.  Name can have,
	 * e.g. '-100' on the end  for 100MHz part.
	 */
	pclk = 55000000;
	if(p = strrchr(ctlr->name, '-'))
		pclk = strtoul(p+1, 0, 0) * 1000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > pclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar mode, x;

	/*
	 * Put the chip to sleep if possible.
	 */
	if(ctlr->name[8] == '1'){
		x = attdaci(Cr0);
		attdaco(Cr0, x|0x04);
	}

	/*
	 * Set the mode in the RAMDAC, setting 6/8-bit colour
	 * as appropriate and waking the chip back up.
	 */
	mode = 0x00;
	if(vga->mode->z == 8 && ctlr->name[8] == '1' && 0)
		mode |= 0x02;
	attdaco(Cr0, mode);
}

static void
dump(Vga*, Ctlr* ctlr)
{
	printitem(ctlr->name, "Cr0");
	printreg(attdaci(Cr0));
}

Ctlr att20c490 = {
	"att20c490",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr att20c491 = {
	"att20c491",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr att20c492 = {
	"att20c492",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
