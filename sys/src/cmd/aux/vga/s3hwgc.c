#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

static void
init(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Finit;

	if(cflag)
		return;
	/*
	 * Use of the on-chip hwgc requires using enhanced mode.
	 */
	if(vga->ctlr == 0 || (vga->ctlr->flag & Henhanced) == 0 || vga->mode->z < 8){
		cflag = 1;
		return;
	}
	resyncinit(vga, ctlr, Uenhanced, 0);
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ctlr->flag |= Fload;

	if(cflag)
		return;
	/*
	 * Use of the on-chip hwgc requires using enhanced mode.
	 */
	if(vga->ctlr == 0 || (vga->ctlr->flag & Uenhanced) == 0 || vga->mode->z < 8)
		cflag = 1;
}

Ctlr bt485hwgc = {
	"bt485hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};

Ctlr rgb524hwgc = {
	"rgb524hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};

Ctlr s3hwgc = {
	"s3hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	0,				/* dump */
};

Ctlr tvp3020hwgc = {
	"tvp3020hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};

Ctlr tvp3026hwgc = {
	"tvp3026hwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
