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
 * Tvp302[056] Viewpoint Video Interface Palette.
 * Assumes hooked up to an S3 86C928 or S3 Vision964.
 */
enum {
	Index		= 0x06,		/* Index register */
	Data		= 0x07,		/* Data register */

	Id		= 0x3F,		/* ID Register */
	Tvp3020		= 0x20,
	Tvp3025		= 0x25,
	Tvp3026		= 0x26,
};

/*
 * The following two arrays give read (bit 0) and write (bit 1)
 * permissions on the direct and index registers. Bits 4 and 5
 * are for the Tvp3025. The Tvp3020 has only 8 direct registers.
 */
static uint8_t directreg[32] = {
	0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x33, 0x33,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x11, 0x33, 0x33, 0x33, 0x33, 0x33,
};

static uint8_t indexreg[64] = {
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x00,
	0x02, 0x02, 0x33, 0x00, 0x00, 0x00, 0x30, 0x30,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x30, 0x00,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x33, 0x33, 0x33, 0x30, 0x30, 0x30, 0x30,
	0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
	0x33, 0x30, 0x33, 0x11, 0x11, 0x11, 0x22, 0x11,
};

/*
 * Check the index register access is valid.
 * Return the number of direct registers.
 */
static uint8_t
checkindex(uint8_t index, uint8_t access)
{
	uint8_t crt55;
	static uint8_t id;

	if(id == 0){
		crt55 = vgaxi(Crtx, 0x55) & 0xFC;
		vgaxo(Crtx, 0x55, crt55|((Index>>2) & 0x03));
		vgao(dacxreg[Index & 0x03], Id);

		id = vgai(dacxreg[Data & 0x03]);
		vgaxo(Crtx, 0x55, crt55);
	}

	if(index == 0xFF)
		return id;

	access &= 0x03;
	switch(id){

	case Tvp3020:
		break;

	case Tvp3025:
	case Tvp3026:
		access = access<<4;
		break;

	default:
		Bprint(&stdout, "%s: unknown chip id - 0x%2.2X\n", tvp3020.name, id);
		break;
	}

	if(index > sizeof(indexreg) || (indexreg[index] & access) == 0)
		error("%s: bad register index - 0x%2.2X\n", tvp3020.name, index);

	return id;
}

static uint8_t
tvp3020io(uint8_t reg, uint8_t data)
{
	uint8_t crt55;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	vgao(dacxreg[reg & 0x03], data);

	return crt55;
}

uint8_t
tvp3020i(uint8_t reg)
{
	uint8_t crt55, r;

	crt55 = vgaxi(Crtx, 0x55) & 0xFC;
	vgaxo(Crtx, 0x55, crt55|((reg>>2) & 0x03));
	r = vgai(dacxreg[reg & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

uint8_t
tvp3020xi(uint8_t index)
{
	uint8_t crt55, r;

	checkindex(index, 0x01);

	crt55 = tvp3020io(Index, index);
	r = vgai(dacxreg[Data & 0x03]);
	vgaxo(Crtx, 0x55, crt55);

	return r;
}

void
tvp3020o(uint8_t reg, uint8_t data)
{
	uint8_t crt55;

	crt55 = tvp3020io(reg, data);
	vgaxo(Crtx, 0x55, crt55);
}

void
tvp3020xo(uint8_t index, uint8_t data)
{
	uint8_t crt55;

	checkindex(index, 0x02);

	crt55 = tvp3020io(Index, index);
	vgao(dacxreg[Data & 0x03], data);
	vgaxo(Crtx, 0x55, crt55);
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Hclk2|Hextsid|Hpvram|Henhanced|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	uint32_t grade;
	char *p;

	/*
	 * Work out the part speed-grade from name. Name can have,
	 * e.g. '-135' on the end  for 135MHz part.
	 */
	grade = 110000000;
	if((p = strrchr(ctlr->name, '-')) != nil)
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
	if(vga->f[0] > 85000000){
		vga->f[0] /= 2;
		resyncinit(vga, ctlr, Uclk2, 0);
	}

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uint8_t x;

	/*
	 * Input Clock Selection:
	 *	VGA		CLK0
	 * 	enhanced	CLK1
	 * doubled if necessary.
	 */
	x = 0x00;
	if(ctlr->flag & Uenhanced)
		x |= 0x01;
	if(ctlr->flag & Uclk2)
		x |= 0x10;
	tvp3020xo(0x1A, x);

	/*
	 * Output Clock Selection:
	 *	VGA		default VGA
	 *	enhanced	RCLK=SCLK=/8, VCLK/4
	 */
	x = 0x3E;
	if(ctlr->flag & Uenhanced)
		x = 0x53;
	tvp3020xo(0x1B, x);

	/*
	 * Auxiliary Control:
	 *	default settings - self-clocked, palette graphics, no zoom
	 * Colour Key Control:
	 *	default settings - pointing to palette graphics
	 * Mux Control Register 1:
	 *	pseudo colour
	 */
	tvp3020xo(0x29, 0x09);
	tvp3020xo(0x38, 0x10);
	tvp3020xo(0x18, 0x80);

	/*
	 * Mux Control Register 2:
	 *	VGA		default VGA
	 *	enhanced	8-bpp, 8:1 mux, 64-bit pixel-bus width
	 */
	x = 0x98;
	if(ctlr->flag & Uenhanced){
		if(vga->mode->z == 8)
			x = 0x1C;
		else if(vga->mode->z == 1)
			x = 0x04;
	}
	tvp3020xo(0x19, x);

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
	tvp3020xo(0x1D, x);
	vga->misc |= 0xC0;
	vgao(MiscW, vga->misc);

	/*
	 * Select the DAC via the General Purpose I/O
	 * Register and Pins.
	 * Guesswork by looking at register dumps.
	 */
	tvp3020xo(0x2A, 0x1F);
	if(ctlr->flag & Uenhanced)
		x = 0x1D;
	else
		x = 0x1C;
	tvp3020xo(0x2B, x);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	uint8_t access;
	int i;

	access = 0x01;
	if(checkindex(0x00, 0x01) != Tvp3020)
		access <<= 4;

	printitem(ctlr->name, "direct");
	for(i = 0; i < 8; i++){
		if(directreg[i] & access)
			printreg(tvp3020i(i));
		else
			printreg(0xFF);
	}

	printitem(ctlr->name, "index");
	for(i = 0; i < sizeof(indexreg); i++){
		if(indexreg[i] & access)
			printreg(tvp3020xi(i));
		else
			printreg(0xFF);
	}
}

Ctlr tvp3020 = {
	"tvp3020",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
