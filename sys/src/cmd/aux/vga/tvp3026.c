#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Tvp3026 Viewpoint Video Interface Palette.
 * Assumes hooked up to an S3 chip of some kind.
 * Why is register access different from the
 * Tvp302[05]?
 */
enum {
	Index		= 0x00,		/* Index register */
	Data		= 0x0A,		/* Data register */

	Id		= 0x3F,		/* ID Register */
	Tvp3026		= 0x26,
};

static uchar
tvp3026io(uchar reg, uchar data)
{
	uchar crt55;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	vgao(dacxreg[reg & 0x03], data);

	return crt55;
}

static uchar
tvp3026i(uchar reg)
{
	uchar crt55, r;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	r = vgai(dacxreg[reg & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

uchar
tvp3026xi(uchar index)
{
	uchar crt55, r;

	crt55 = tvp3026io(Index, index);
	vgaxo(Crtx, 0x55, crt55|((Data>>2) & 0x03));
	r = vgai(dacxreg[Data & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

static void
tvp3026o(uchar reg, uchar data)
{
	uchar crt55;

	crt55 = tvp3026io(reg, data);
	vgaxo(Crtx, 0x55, crt55);
}

void
tvp3026xo(uchar index, uchar data)
{
	uchar crt55;

	crt55 = tvp3026io(Index, index);
	vgaxo(Crtx, 0x55, crt55|((Data>>2) & 0x03));
	vgao(dacxreg[Data & 0x03], data);
	vgaxo(Crtx, 0x55, crt55);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hclk2|Hextsid|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong grade;
	char *p;

	/*
	 * Work out the part speed-grade from name. Name can have,
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
	if((ctlr->flag & Uclk2) == 0 && vga->mode->z == 8 && vga->f[0] > 85000000)
		resyncinit(vga, ctlr, Uclk2, 0);

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar x;

	/*
	 * General Control:
	 *	output sync polarity
	 * It's important to set this properly and to turn off the
	 * VGA controller H and V syncs. Can't be set in VGA mode.
	 */
	x = 0x00;
	if((vga->misc & 0x40) == 0)
		x |= 0x01;
	if((vga->misc & 0x80) == 0)
		x |= 0x02;
	tvp3026xo(0x1D, x);
	vga->misc |= 0xC0;
	vgao(MiscW, vga->misc);

	ctlr->flag |= Fload;
}

static void
dump(Vga*, Ctlr* ctlr)
{
	int i;
	ulong clock[4], f;

	printitem(ctlr->name, "direct");
	for(i = 0; i < 16; i++)
		printreg(tvp3026i(i));

	printitem(ctlr->name, "index");
	for(i = 0; i < 64; i++)
		printreg(tvp3026xi(i));

	printitem(ctlr->name, "PCLK");
	for(i = 0; i < 3; i++){
		tvp3026xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(clock[i] = tvp3026xi(0x2D));
	}
	f = (RefFreq*(65-clock[1]))/(65-(clock[0] & 0x3F));
	f >>= clock[2] & 0x03;
	Bprint(&stdout, "%23ld", f);

	printitem(ctlr->name, "MCLK");
	for(i = 0; i < 3; i++){
		tvp3026xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(clock[i] = tvp3026xi(0x2E));
	}
	f = (RefFreq*(65-clock[1]))/(65-(clock[0] & 0x3F));
	f >>= clock[2] & 0x03;
	Bprint(&stdout, "%23ld", f);

	printitem(ctlr->name, "LCLK");
	for(i = 0; i < 3; i++){
		tvp3026xo(0x2C, (i<<4)|(i<<2)|i);
		printreg(clock[i] = tvp3026xi(0x2F));
	}
	Bprint(&stdout, "\n");
}

Ctlr tvp3026 = {
	"tvp3026",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	0,				/* load */
	dump,				/* dump */
};
