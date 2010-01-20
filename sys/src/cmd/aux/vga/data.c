#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

int cflag;					/* do not use hwgc */
int dflag;					/* do the palette */

Ctlr* ctlrs[] = {
	&ark2000pv,				/* ctlr */
	&ark2000pvhwgc,				/* hwgc */
	&att20c490,				/* ramdac */
	&att20c491,				/* ramdac */
	&att20c492,				/* ramdac */
	&att21c498,				/* ramdac */
	&bt485,					/* ramdac */
	&bt485hwgc,				/* hwgc */
	&ch9294,				/* clock */
	&clgd542x,				/* ctlr */
	&clgd542xhwgc,				/* hwgc */
	&clgd546x,				/* ctlr */
	&clgd546xhwgc,				/* hwgc */
	&ct65540,				/* ctlr */
	&ct65545,				/* ctlr */
	&ct65545hwgc,				/* hwgc */
	&cyber938x,				/* ctlr */
	&cyber938xhwgc,				/* hwgc */
	&et4000,				/* ctlr */
	&et4000hwgc,				/* hwgc */
	&generic,				/* ctlr */
	&hiqvideo,				/* ctlr */
	&hiqvideohwgc,				/* hwgc */
	&i81x,				/* ctlr */
	&i81xhwgc,				/* hwgc */
	&ibm8514,				/* ctlr */
	&icd2061a,				/* clock */
	&ics2494,				/* clock */
	&ics2494a,				/* clock */
	&ics534x,				/* gendac */
	&mach32,				/* ctlr */
	&mach64,				/* ctlr */
	&mach64xx,				/* ctlr */
	&mach64xxhwgc,				/* hwgc */
	&mga2164w,				/* ctlr */
	&mga2164whwgc,				/* hwgc */
	&neomagic,				/* ctlr */
	&neomagichwgc,				/* hwgc */
	&nvidia,				/* ctlr */
	&nvidiahwgc,				/* hwgc */
	&radeon,				/* ctlr */
	&radeonhwgc,				/* hwgc */
	&palette,				/* ctlr */
	&rgb524,				/* ramdac */
	&rgb524hwgc,				/* hwgc */
	&rgb524mn,				/* ramdac */
	&s3801,					/* ctlr */
	&s3805,					/* ctlr */
	&s3928,					/* ctlr */
	&s3clock,				/* clock */
	&s3hwgc,				/* hwgc */
	&sc15025,				/* ramdac */
	&softhwgc,				/* hwgc */
	&stg1702,				/* ramdac */
	&t2r4,					/* ctlr */
	&t2r4hwgc,				/* hwgc */
	&tdfx,					/* ctlr */
	&tdfxhwgc,				/* hwgc */
	&trio64,				/* ctlr */
	&tvp3020,				/* ramdac */
	&tvp3020hwgc,				/* hwgc */
	&tvp3025,				/* ramdac */
	&tvp3025clock,				/* clock */
	&tvp3026,				/* ramdac */
	&tvp3026clock,				/* clock */
	&tvp3026hwgc,				/* hwgc */
	&vesa,					/* ctlr */
	&virge,					/* ctlr */
	&vision864,				/* ctlr */
	&vision964,				/* ctlr */
	&vision968,				/* ctlr */
	&vmware,				/* ctlr */
	&vmwarehwgc,				/* hwgc */
	&w30c516,				/* ctlr */
	&mga4xx,
	&mga4xxhwgc,
	0,
};

/*
 * Lower 2-bits of indirect DAC register
 * addressing.
 */
ushort dacxreg[4] = {
	PaddrW, Pdata, Pixmask, PaddrR
};
