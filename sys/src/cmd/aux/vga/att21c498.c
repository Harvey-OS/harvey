#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ATT21C498 16-Bit Interface RAMDAC
 * PrecisionDAC Technology
 */
enum {
	Cr0		= 0x00,		/* Control register 0 */
	Midr		= 0x01,		/* Manufacturer's identification register */
	Didr		= 0x02,		/* Device identification register */
	Rtest		= 0x03,		/* Red signature analysis register */
	Gtest		= 0x04,		/* Green signature analysis register */
	Btest		= 0x05,		/* Blue signature analysis register */

	Nir		= 0x06,		/* number of indirect registers */
};

static void
attdacio(uchar reg)
{
	int i;

	/*
	 * Access to the indirect registers is accomplished by reading
	 * Pixmask 4 times, then subsequent reads cycle through the
	 * indirect registers. Any I/O write to a direct register resets
	 * the sequence.
	 */
	inportb(PaddrW);
	for(i = 0; i < 4+reg; i++)
		inportb(Pixmask);
}

uchar
attdaci(uchar reg)
{
	uchar r;

	attdacio(reg);
	r = inportb(Pixmask);

	inportb(PaddrW);
	return r;
}

void
attdaco(uchar reg, uchar data)
{
	attdacio(reg);
	outportb(Pixmask, data);
	inportb(PaddrW);
}

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
	 * Part comes in -170, -130 and -110MHz speed-grades.
	 * In 8-bit mode the max. PCLK is 80MHz for the -110 part
	 * and 110MHz for the others. In 2x8-bit mode, the max.
	 * PCLK is the speed-grade, using the 2x doubler.
	 * We can use mode 2 (2x8-bit, internal clock doubler)
	 * if connected to a suitable graphics chip, e.g. the
	 * S3 Vision864.
	 * Work out the part speed-grade from name.  Name can have,
	 * e.g. '-135' on the end  for 135MHz part.
	 */
	grade = 110000000;
	if(p = strrchr(ctlr->name, '-'))
		grade = strtoul(p+1, 0, 0) * 1000000;

	if(vga->ctlr && ((vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8))
		pclk = grade;
	else{
		if(grade == 110000000)
			pclk = 80000000;
		else
			pclk = 110000000;
	}

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
	 * initialise it again.
	 */
	if(vga->ctlr && (vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8 && vga->f[0] > 80000000){
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
	x = attdaci(Cr0);
	attdaco(Cr0, x|0x04);

	mode = 0x00;
	if(ctlr->flag & Upclk2x8)
		mode = 0x20;

	/*
	 * Set the mode in the RAMDAC, setting 6/8-bit colour
	 * as appropriate and waking the chip back up.
	 */
	if(vga->mode->z == 8 && 0)
		mode |= 0x02;
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

Ctlr att21c498 = {
	"att21c498",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
