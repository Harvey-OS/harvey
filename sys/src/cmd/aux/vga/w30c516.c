#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * IC Works W30C516 ZOOMDAC.
 * DSP-based Multimedia RAMDAC.
 */
enum {
	Cr0		= 0x00,		/* Control register 0 */
	Mid		= 0x01,		/* Manufacturer's identification register */
	Did		= 0x02,		/* Device identification register */
	Cr1		= 0x03,		/* Control register 1 */

	Reserve1	= 0x04,		/* Reserved (16-bit) */
	Reserve2	= 0x06,		/* Reserved (16-bit) */
	Reserve3	= 0x08,		/* Reserved (16-bit) */
	Reserve4	= 0x0A,		/* Reserved (16-bit) */

	IstartX		= 0x0C,		/* Image Start X (16-bit) */
	IstartY		= 0x0E,		/* Image Start Y (16-bit) */
	IendX		= 0x10,		/* Image End X (16-bit) */
	IendY		= 0x12,		/* Image End Y (16-bit) */

	RatioX		= 0x14,		/* Ratio X (16-bit) */
	RatioY		= 0x16,		/* Ratio Y (16-bit) */
	OffsetX		= 0x18,		/* Offset X (16-bit) */
	OffsetY		= 0x1A,		/* Offset Y (16-bit) */

	TestR		= 0x1C,		/* Red signature analysis register */
	TestG		= 0x1D,		/* Green signature analysis register */
	TestB		= 0x1E,		/* Blue signature analysis register */

	Nir		= 0x1F,		/* number of indirect registers */
};

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hpclk2x8|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong grade, pclk;
	char *p;

	/*
	 * Part comes in -170, -135 and -110MHz speed-grades.
	 * In 8-bit mode the max. PCLK is 135MHz for the -170 part
	 * and the speed-grade for the others. In 2x8-bit mode, the max.
	 * PCLK is the speed-grade, using the 2x doubler.
	 * Work out the part speed-grade from name. Name can have,
	 * e.g. '-135' on the end  for 135MHz part.
	 */
	grade = 110000000;
	if(p = strrchr(ctlr->name, '-'))
		grade = strtoul(p+1, 0, 0) * 1000000;

	if(grade == 170000000)
		pclk = 135000000;
	else
		pclk = grade;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	/*
	 * Determine whether to use 2x8-bit mode or not.
	 * If yes and the clock has already been initialised,
	 * initialise it again. There is no real frequency
	 * restriction, it's really just a lower limit on what's
	 * available in some clock generator chips.
	 */
	if(vga->ctlr && (vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8 && vga->f[0] >= 60000000){
		vga->f[0] /= 2;
		resyncinit(vga, ctlr, Upclk2x8, 0);
	}
	if(vga->f[0] > pclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar mode, x;

	/*
	 * Put the chip to sleep.
	 */
	attdaco(Cr0, 0x08);

	mode = 0x00;
	if(ctlr->flag & Upclk2x8)
		mode = 0x20;

	/*
	 * Set the mode in the RAMDAC, setting 6/8-bit colour
	 * as appropriate and waking the chip back up.
	 */
	if(vga->mode->z == 8 && 0)
		mode |= 0x02;
	x = attdaci(Cr1) & 0x80;
	attdaco(Cr1, x);
	attdaco(Cr0, mode);

	ctlr->flag |= Fload;
}

static void
dump(Vga*, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "");
	for(i = 0; i < Nir; i++)
		printreg(attdaci(i));
}

Ctlr w30c516 = {
	"w30c516",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
