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

int cflag;					/* do not use hwgc */
int dflag;					/* do the palette */

Ctlr* ctlrs[] = {
	&ch9294,				/* clock */
	&clgd542x,				/* ctlr */
	&clgd542xhwgc,				/* hwgc */
	&clgd546x,				/* ctlr */
	&clgd546xhwgc,				/* hwgc */
	&generic,				/* ctlr */
	&icd2061a,				/* clock */
	&ics2494,				/* clock */
	&ics2494a,				/* clock */
	&ics534x,				/* gendac */
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
	0,
};

/*
 * Lower 2-bits of indirect DAC register
 * addressing.
 */
uint16_t dacxreg[4] = {
	PaddrW, Pdata, Pixmask, PaddrR
};
