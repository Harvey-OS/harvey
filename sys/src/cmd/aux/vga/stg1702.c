#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * SGS-Thompson STG1702 Enhanced True Color Palette-DAC
 * with 16-bit Pixel Port.
 */
enum {
	Command		= 0x00,		/* Pixel Command Register */
	IndexLO		= 0x01,		/* LO-byte of 16-bit Index */
	IndexHI		= 0x02,		/* HI-byte of 16-bit Index */
	Index		= 0x03,		/* Indexed Register */

	CompanyID	= 0x00,		/* Company ID = 0x44 */
	DeviceID	= 0x01,		/* Device ID = 0x02 */
	Pmode		= 0x03,		/* Primary Pixel Mode Select */
	Smode		= 0x04,		/* Secondary Pixel Mode Select */
	Pipeline	= 0x05,		/* Pipeline Timing Control */
	Sreset		= 0x06,		/* Soft Reset */
	Power		= 0x07,		/* Power Management */

	Nindex		= 0x08,
};

static void
pixmask(void)
{
	inportb(PaddrW);
}

static void
commandrw(void)
{
	int i;

	pixmask();
	for(i = 0; i < 4; i++)
		inportb(Pixmask);
}

static uchar
commandr(void)
{
	uchar command;

	commandrw();
	command = inportb(Pixmask);
	pixmask();

	return command;
}

static void
commandw(uchar command)
{
	commandrw();
	outportb(Pixmask, command);
	pixmask();
}

static void
indexrw(uchar index)
{
	uchar command;

	command = commandr();
	commandw(command|0x10);

	commandrw();
	inportb(Pixmask);
	outportb(Pixmask, index & 0xFF);
	outportb(Pixmask, (index>>8) & 0xFF);
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hpclk2x8|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong pclk;

	/*
	 * Part comes in -135MHz speed-grade.
	 * In 8-bit mode the max. PCLK is 110MHz. In 2x8-bit mode
	 * the max. PCLK is the speed-grade, using the 2x doubler.
	 * We can use mode 2 (2x8-bit, internal clock doubler)
	 * if connected to a suitable graphics chip, e.g. the
	 * ET4000-w32p.
	 */
	if(vga->ctlr && ((vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8))
		pclk = 135000000;
	else
		pclk = 110000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] < 16000000 || vga->f[0] > pclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);

	/*
	 * Determine whether to use 2x8-bit mode or not.
	 * If yes and the clock has already been initialised,
	 * initialise it again.
	 */
	if(vga->ctlr && (vga->ctlr->flag & Hpclk2x8) && vga->mode->z == 8 && vga->f[0] >= 110000000){
		vga->f[0] /= 2;
		resyncinit(vga, ctlr, Upclk2x8, 0);
	}

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar command, mode, pipeline;

	command = 0x00;
	mode = 0x00;
	pipeline = 0x02;
	if(ctlr->flag & Upclk2x8){
		command = 0x08;
		mode = 0x05;
		pipeline = 0x02;
		if(vga->f[0] < 16000000)
			pipeline = 0x00;
		else if(vga->f[0] < 32000000)
			pipeline = 0x01;
	}

	indexrw(Pmode);
	outportb(Pixmask, mode);
	outportb(Pixmask, mode);
	outportb(Pixmask, pipeline);
	sleep(1);
	commandw(command);

	ctlr->flag |= Fload;
}

static void
dump(Vga*, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "command");
	printreg(commandr());

	printitem(ctlr->name, "index");
	indexrw(CompanyID);
	for(i = 0; i < Nindex; i++)
		printreg(inportb(Pixmask));

	pixmask();
}

Ctlr stg1702 = {
	"stg1702",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
