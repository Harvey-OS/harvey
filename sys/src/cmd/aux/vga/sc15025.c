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
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong pclk;
	char *p;

	/*
	 * Part comes in -125, -110, -80, and -66MHz speed-grades.
	 * Work out the part speed-grade from name.  Name can have,
	 * e.g. '-110' on the end  for 100MHz part.
	 */
	pclk = 66000000;
	if(p = strrchr(ctlr->name, '-'))
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
load(Vga*, Ctlr*)
{
	uchar aux, command;

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
dump(Vga*, Ctlr* ctlr)
{
	int i;
	uchar command;

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
