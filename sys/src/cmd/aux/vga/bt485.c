#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Brooktree Bt485 Monolithic True-Color RAMDAC.
 * Assumes hooked up to an S3 86C928.
 *
 * There are only 16 directly addressable registers,
 * Cmd3 is accessed indirectly and overlays the
 * status register. We make this register index 0x1A.
 * Cmd4 is an artifact of the Tvp3025 in Bt485
 * emulation mode.
 */
enum {
	AddrW		= 0x00,		/* Address register; palette/cursor RAM write */
	Palette		= 0x01,		/* 6/8-bit color palette data */
	Pmask		= 0x02,		/* Pixel mask register */
	AddrR		= 0x03,		/* Address register; palette/cursor RAM read */
	ColorW		= 0x04,		/* Address register; cursor/overscan color write */
	Color		= 0x05,		/* Cursor/overscan color data */
	Cmd0		= 0x06,		/* Command register 0 */
	ColorR		= 0x07,		/* Address register; cursor/overscan color read */
	Cmd1		= 0x08,		/* Command register 1 */
	Cmd2		= 0x09,		/* Command register 2 */
	Status		= 0x0A,		/* Status */
	Cram		= 0x0B,		/* Cursor RAM array data */
	Cxlr		= 0x0C,		/* Cursor x-low register */
	Cxhr		= 0x0D,		/* Cursor x-high register */
	Cylr		= 0x0E,		/* Cursor y-low register */
	Cyhr		= 0x0F,		/* Cursor y-high register */

	Nreg		= 0x10,

	Cmd3		= 0x1A,		/* Command register 3 */
	Cmd4		= 0x2A,		/* Command register 4 */
};

static uchar
bt485io(uchar reg)
{
	uchar crt55, cr0;

	if(reg >= Nreg && (reg & 0x0F) != Status)
		error("%s: bad reg - 0x%X\n", bt485.name, reg);

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	if((reg & 0x0F) == Status){
		/*
		 * 1,2: Set indirect addressing for Status or
		 *      Cmd[34] - set bit7 of Cr0.
		 */
		vgaxo(Crtx, 0x55, crt55|((Cmd0>>2) & 0x03));
		cr0 = vgai(dacxreg[Cmd0 & 0x03])|0x80;
		vgao(dacxreg[Cmd0 & 0x03], cr0);

		/*
		 * 3,4: Set the index into the Write register,
		 *      index == 0x00 for Status, 0x01 for Cmd3,
		 *		 0x02 for Cmd4.
		 */
		vgaxo(Crtx, 0x55, crt55|((AddrW>>2) & 0x03));
		vgao(dacxreg[AddrW & 0x03], (reg>>4) & 0x0F);

		/*
		 * 5,6: Get the contents of the appropriate
		 *      register at 0x0A.
		 */
	}

	return crt55;
}

uchar
bt485i(uchar reg)
{
	uchar crt55, r;

	crt55 = bt485io(reg);
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	r = vgai(dacxreg[reg & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

void
bt485o(uchar reg, uchar data)
{
	uchar crt55;

	crt55 = bt485io(reg);
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	vgao(dacxreg[reg & 0x03], data);
	vgaxo(Crtx, 0x55, crt55);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hsid32|Hclk2|Hextsid|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong grade;
	char *p;

	/*
	 * Work out the part speed-grade from name.  Name can have,
	 * e.g. '-135' on the end  for 135MHz part.
	 */
	grade = 110000000;
	if(p = strrchr(ctlr->name, '-'))
		grade = strtoul(p+1, 0, 0) * 1000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > grade)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);

	/*
	 * Determine whether to use clock-doubler or not.
	 */
	if((ctlr->flag & Uclk2) == 0 && vga->f[0] > 67500000){
		vga->f[0] /= 2;
		resyncinit(vga, ctlr, Uclk2, 0);
	}

	ctlr->flag |= Finit;
}

static void
load(Vga*, Ctlr* ctlr)
{
	uchar x;

	/*
	 * Put the chip to sleep before (possibly) changing the clock-source
	 * to ensure the integrity of the palette.
	 */
	x = bt485i(Cmd0);
	bt485o(Cmd0, x|0x01);				/* sleep mode */

	/*
	 * Set the clock-source, clock-doubler and frequency
	 * appropriately:
	 *	if using the pixel-port, use Pclock1;
	 *	use the doubler if fvco > 67.5MHz.
	 *	set the frequency.
	 */
	x = bt485i(Cmd2);
	if(ctlr->flag & Uenhanced)
		x |= 0x10;				/* Pclock1 */
	else
		x &= ~0x10;
	bt485o(Cmd2, x);

	x = bt485i(Cmd3);
	if(ctlr->flag & Uclk2)
		x |= 0x08;				/* clock-doubler */
	else
		x &= ~0x08;
	bt485o(Cmd3, x);

	/*
	 * Set 4:1 multiplex and portselect bits on the
	 * Bt485 appropriately for using the pixel-port.
	 */
	x = bt485i(Cmd2);
	if(ctlr->flag & Uenhanced){
		bt485o(Cmd1, 0x40);			/* 4:1 multiplex */
		x |= 0x20;				/* portselect */
	}
	else{
		bt485o(Cmd1, 0x00);
		x &= ~0x20;
	}
	bt485o(Cmd2, x);

	x = bt485i(Cmd4);
	if(ctlr->flag & Uenhanced)
		x |= 0x01;
	else
		x &= ~0x01;
	bt485o(Cmd4, x);

	/*
	 * All done, wake the chip back up.
	 * Set 6/8-bit colour as appropriate.
	 */
	x = bt485i(Cmd0) & ~0x01;
	/*if(vga->mode.z == 8)
		x |= 0x02;
	else*/
		x &= ~0x02;
	bt485o(Cmd0, x);

	ctlr->flag |= Fload;
}

static void
dump(Vga*, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "direct");
	for(i = 0; i < 0x10; i++)
		printreg(bt485i(i));
	printitem(ctlr->name, "indirect");
	printreg(bt485i(Cmd3));
	printreg(bt485i(Cmd4));
}

Ctlr bt485 = {
	"bt485",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
