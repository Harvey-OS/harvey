#include <u.h>
#include <libc.h>

#include "vga.h"

static void
setcrt42(Vga *vga, Ctlr *ctlr, uchar index)
{
	verbose("%s->clock->setcrt42\n", ctlr->name);

	vgao(MiscW, vga->misc & ~0x0C);
	outportb(Crtx+1, 0x00);

	vga->crt[0x42] = (vga->crt[0x42] & 0xF0)|index;
	vgao(MiscW, vga->misc);
	vgaxo(Crtx, 0x42, vga->crt[0x42]);
}

static void
icd2061aload(Vga *vga, Ctlr *ctlr)
{
	ulong sdata;
	int i;
	uchar crt42;

	verbose("%s->clock->icd2061aload\n", ctlr->name);
	/*
	 * The serial word to be loaded into the icd2061a is
	 *	(2<<21)|(vga->i<<17)|((vga->n)<<10)|(vga->p<<7)|vga->d
	 * Always select ICD2061A REG2.
	 */
	sdata = (2<<21)|(vga->i<<17)|((vga->n)<<10)|(vga->p<<7)|vga->d;

	/*
	 * The display should be already off to enable  us to clock the
	 * serial data word into either MiscW or Crt42.
	 *
	 * Set the Misc register to make clock-select-out the contents of
	 * register Crt42. Must turn the sequencer off when changing bits
	 * <3:2> of Misc. Also, must set Crt42 to 0 before setting <3:2>
	 * of Misc due to a hardware glitch.
	 */
	vgao(MiscW, vga->misc & ~0x0C);
	crt42 = vgaxi(Crtx, 0x42) & 0xF0;
	outportb(Crtx+1, 0x00);

	vgaxo(Seqx, 0x00, 0x00);
	vgao(MiscW, vga->misc|0x0C);
	vgaxo(Seqx, 0x00, 0x03);

	/*
	 * Unlock the ICD2061A. The unlock sequence is at least 5 low-to-high
	 * transitions of CLK with DATA high, followed by a single low-to-high
	 * transition of CLK with DATA low.
	 * Using Crt42, CLK is bit0, DATA is bit 1. If we used MiscW, they'd
	 * be bits 2 and 3 respectively.
	 */
	outportb(Crtx+1, crt42|0x00);			/* -DATA|-CLK */
	outportb(Crtx+1, crt42|0x02);			/* +DATA|-CLK */
	for(i = 0; i < 5; i++){
		outportb(Crtx+1, crt42|0x03);		/* +DATA|+CLK */
		outportb(Crtx+1, crt42|0x02);		/* +DATA|-CLK */
	}
	outportb(Crtx+1, crt42|0x00);			/* -DATA|-CLK */
	outportb(Crtx+1, crt42|0x01);			/* -DATA|+CLK */

	/*
	 * Now write the serial data word, framed by a start-bit and a stop-bit.
	 * The data is written using a modified Manchester encoding such that a
	 * data-bit is sampled on the rising edge of CLK and the complement of
	 * the data-bit is sampled on the previous falling edge of CLK.
	 */
	outportb(Crtx+1, crt42|0x00);			/* -DATA|-CLK (start-bit) */
	outportb(Crtx+1, crt42|0x01);			/* -DATA|+CLK */

	for(i = 0; i < 24; i++){			/* serial data word */
		if(sdata & 0x01){
			outportb(Crtx+1, crt42|0x01);	/* -DATA|+CLK */
			outportb(Crtx+1, crt42|0x00);	/* -DATA|-CLK (falling edge) */
			outportb(Crtx+1, crt42|0x02);	/* +DATA|-CLK */
			outportb(Crtx+1, crt42|0x03);	/* +DATA|+CLK (rising edge) */
		}
		else {
			outportb(Crtx+1, crt42|0x03);	/* +DATA|+CLK */
			outportb(Crtx+1, crt42|0x02);	/* +DATA|-CLK (falling edge) */
			outportb(Crtx+1, crt42|0x00);	/* -DATA|-CLK */
			outportb(Crtx+1, crt42|0x01);	/* -DATA|+CLK (rising edge) */
		}
		sdata >>= 1;
	}

	outportb(Crtx+1, crt42|0x03);			/* +DATA|+CLK (stop-bit) */
	outportb(Crtx+1, crt42|0x02);			/* +DATA|-CLK */
	outportb(Crtx+1, crt42|0x03);			/* +DATA|+CLK */

	/*
	 * We always use REG2 in the ICD2061A.
	 */
	setcrt42(vga, ctlr, 0x02);
}

static void
ch9294load(Vga *vga, Ctlr *ctlr)
{
	verbose("%s->clock->ch9294load\n", ctlr->name);

	setcrt42(vga, ctlr, vga->i);
}

static struct {
	char*	name;
	void	(*load)(Vga*, Ctlr*);
} clocks[] = {
	{ "icd2061a",	icd2061aload, },
	{ "ch9294",	ch9294load, },
	{ 0 },
};

static void
init(Vga *vga, Ctlr *ctlr)
{
	char name[NAMELEN+1], *p;
	int i;

	verbose("%s->init\n", ctlr->name);

	if(vga->clock == 0)
		return;

	/*
	 * Check we know about it.
	 */
	strncpy(name, vga->clock->name, NAMELEN);
	name[NAMELEN] = 0;
	if(p = strchr(name, '-'))
		*p = 0;
	for(i = 0; clocks[i].name; i++){
		if(strcmp(clocks[i].name, name) == 0)
			break;
	}
	if(clocks[i].name == 0)
		error("don't recognise s3clock \"%s\"\n", vga->clock->name);

	if(vga->clock->init && (vga->clock->flag & Finit) == 0){
		(*vga->clock->init)(vga, vga->clock);
		(*vga->clock->init)(vga, vga->clock);
		(*vga->clock->init)(vga, vga->clock);
	}

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 */
	if(vga->f == 0)
		vga->f = vga->mode->frequency;
	if(vga->f != VgaFreq0 && vga->f != VgaFreq1)
		vga->misc |= 0x0C;

	ctlr->flag |= Finit;
}

static void
load(Vga *vga, Ctlr *ctlr)
{
	char name[NAMELEN+1], *p;
	int i;

	verbose("%s->load\n", ctlr->name);

	if(vga->clock == 0 || (vga->clock->flag & Fload))
		return;

	strncpy(name, vga->clock->name, NAMELEN);
	name[NAMELEN] = 0;
	if(p = strchr(name, '-'))
		*p = 0;

	for(i = 0; clocks[i].name; i++){
		if(strcmp(clocks[i].name, name))
			continue;
		(*clocks[i].load)(vga, ctlr);

		ctlr->flag |= Fload;
		return;
	}
}

Ctlr s3clock = {
	"s3clock",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	0,				/* dump */
};
