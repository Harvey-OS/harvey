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
 * Sierra SC1502[56] HiCOLOR-24 Palette.
 */
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

static uint8_t
commandr(void)
{
	uint8_t command;

	commandrw();
	command = inportb(Pixmask);
	pixmask();

	return command;
}

static void
commandw(uint8_t command)
{
	commandrw();
	outportb(Pixmask, command);
	pixmask();
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	uint32_t pclk;
	char *p;

	/*
	 * Part comes in -125, -110, -80, and -66MHz speed-grades.
	 * Work out the part speed-grade from name.  Name can have,
	 * e.g. '-110' on the end  for 100MHz part.
	 */
	pclk = 66000000;
	if((p = strrchr(ctlr->name, '-')) != nil)
		pclk = strtoul(p+1, 0, 0) * 1000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > pclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uint8_t aux, command;

	aux = 0x00;
	/*
	if(vga->mode->z == 8)
		aux = 0x01;
	 */
	commandrw();
	command = inportb(Pixmask);
	outportb(Pixmask, command|0x18);
	outportb(PaddrR, 0x08);
	outportb(PaddrW, aux);
	commandw(command);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	uint8_t command;

	printitem(ctlr->name, "command");
	command = commandr();
	printreg(command);

	printitem(ctlr->name, "index08");
	commandw(command|0x10);
	for(i = 0x08; i < 0x11; i++){
		outportb(PaddrR, i);
		printreg(inportb(PaddrW));
	}
	commandw(command);
}

Ctlr sc15025 = {
	"sc15025",			/* name */
	0,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
