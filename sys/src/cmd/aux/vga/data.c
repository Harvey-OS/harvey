#include <u.h>
#include <libc.h>

#include "vga.h"

int cflag;					/* do not use hwgc */
int dflag;					/* do the palette */

Ctlr *ctlrs[] = {
	&att20c491,				/* ramdac */
	&att20c492,				/* ramdac */
	&att21c498,				/* ramdac */
	&bt485,					/* ramdac */
	&bt485hwgc,				/* hwgc */
	&ch9294,				/* clock */
	&clgd542x,				/* ctlr */
	&et4000,				/* ctlr */
	&et4000hwgc,				/* hwgc */
	&generic,				/* ctlr */
	&ibm8514,				/* ctlr */
	&icd2061a,				/* clock */
	&ics2494,				/* clock */
	&ics2494a,				/* clock */
	&mach32,				/* ctlr */
	&mach64,				/* ctlr */
	&palette,				/* ctlr */
	&s3801,					/* ctlr */
	&s3805,					/* ctlr */
	&s3928,					/* ctlr */
	&s3clock,				/* clock */
	&s3hwgc,				/* hwgc */
	&sc15025,				/* ramdac */
	&stg1702,				/* ramdac */
	&tvp3020,				/* ramdac */
	&tvp3020hwgc,				/* hwgc */
	&tvp3025,				/* ramdac */
	&tvp3025clock,				/* clock */
	&vision864,				/* ctlr */
	0,
};

/*
 * Lower 2-bits of indirect DAC register
 * addressing.
 */
ushort dacxreg[4] = {
	PaddrW, Pdata, Pixmask, PaddrR
};
