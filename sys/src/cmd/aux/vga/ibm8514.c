#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * IBM 8514/A Graphics Coprocessor.
 */
enum {
	Subsys		= 0x42E8,	/* Subsystem Status (R), Control (W) */
	Advfunc		= 0x4AE8,	/* Advanced Function Control */
	CurY		= 0x82E8,	/* Current Y Position */
	CurX		= 0x86E8,	/* Current X Position */
	DestyAxstp	= 0x8AE8,	/* Destination Y Position/Axial Step Constant */
	DestxDiastp	= 0x8EE8,	/* Destination X Position/Diagonal Step Constant */
	ErrTerm		= 0x92E8,	/* Error Term */
	MajAxisPcnt	= 0x96E8,	/* Major Axis Pixel Count */
	GPstat		= 0x9AE8,	/* Graphics Processor Status (R) */
	Cmd		= 0x9AE8,	/* Drawing Command (W) */
	ShortStroke	= 0x9EE8,	/* Short Stroke Vector (W) */
	BkgdColor	= 0xA2E8,	/* Background Colour */
	FrgdColor	= 0xA6E8,	/* Foreground Colour */
	WrtMask		= 0xAAE8,	/* Bitplane Write Mask */
	RdMask		= 0xAEE8,	/* Bitplane Read Mask */
	ColorCmp	= 0xB2E8,	/* Colour Compare */
	BkgdMix		= 0xB6E8,	/* Background Mix */
	FrgdMix		= 0xBAE8,	/* Foreground Mix */
	Multifunc	= 0xBEE8,	/* Multifunction Control */
	PixTrans	= 0xE2E8,	/* Pixel Data Transfer */
};

enum {					/* Multifunc Index */
	MinAxisPcnt	= 0x0000,	/* Minor Axis Pixel Count */
	ScissorsT	= 0x1000,	/* Top Scissors */
	ScissorsL	= 0x2000,	/* Left Scissors */
	ScissorsB	= 0x3000,	/* Bottom Scissors */
	ScissorsR	= 0x4000,	/* Right Scissors */
	MemCntl		= 0x5000,	/* Memory Control */
	PixCntl		= 0xA000,	/* Pixel Control */
	MultMisc	= 0xE000,	/* Miscellaneous Multifunction Control (S3) */
	ReadSel		= 0xF000,	/* Read Register Select (S3) */
};

static void
load(Vga* vga, Ctlr*)
{
	outportw(Pixmask, 0x00);
	outportw(Subsys, 0x8000|0x1000);
	outportw(Subsys, 0x4000|0x1000);
	outportw(Pixmask, 0xFF);

	outportw(FrgdMix, 0x47);
	outportw(BkgdMix, 0x07);

	outportw(Multifunc, ScissorsT|0x000);
	outportw(Multifunc, ScissorsL|0x000);
	outportw(Multifunc, ScissorsB|(vga->vmz/vga->mode->x-1));
	outportw(Multifunc, ScissorsR|(vga->mode->x-1));

	outportw(WrtMask, 0xFFFF);
	outportw(Multifunc, PixCntl|0x0000);
}

static void
dump(Vga*, Ctlr* ctlr)
{
	printitem(ctlr->name, "Advfunc");
	Bprint(&stdout, "%9.4uX\n", inportw(Advfunc));
	printitem(ctlr->name, "Subsys");
	Bprint(&stdout, "%9.4uX\n", inportw(Subsys));
}

Ctlr ibm8514 = {
	"ibm8514",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	load,				/* load */
	dump,				/* dump */
};
