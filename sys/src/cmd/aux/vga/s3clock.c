#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Clocks which require fiddling with the S3 registers
 * in order to be loaded.
 */
static void
setcrt42(Vga* vga, Ctlr* ctlr, uchar index)
{
	trace("%s->clock->setcrt42\n", ctlr->name);

	vgao(MiscW, vga->misc & ~0x0C);
	outportb(Crtx+1, 0x00);

	vga->crt[0x42] = (vga->crt[0x42] & 0xF0)|index;
	vgao(MiscW, vga->misc);
	vgaxo(Crtx, 0x42, vga->crt[0x42]);
}

static void
icd2061aload(Vga* vga, Ctlr* ctlr)
{
	ulong sdata;
	int i;
	uchar crt42;

	trace("%s->clock->icd2061aload\n", ctlr->name);
	/*
	 * The serial word to be loaded into the icd2061a is
	 *	(2<<21)|(vga->i<<17)|((vga->n)<<10)|(vga->p<<7)|vga->d
	 * Always select ICD2061A REG2.
	 */
	sdata = (2<<21)|(vga->i[0]<<17)|((vga->n[0])<<10)|(vga->p[0]<<7)|vga->d[0];

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
ch9294load(Vga* vga, Ctlr* ctlr)
{
	trace("%s->clock->ch9294load\n", ctlr->name);

	setcrt42(vga, ctlr, vga->i[0]);
}

static void
tvp3025load(Vga* vga, Ctlr* ctlr)
{
	uchar crt5c, x;

	trace("%s->clock->tvp3025load\n", ctlr->name);

	/*
	 * Crt5C bit 5 is RS4.
	 * Clear it to select TVP3025 registers for
	 * the calls to tvp302xo().
	 */
	crt5c = vgaxi(Crtx, 0x5C);
	vgaxo(Crtx, 0x5C, crt5c & ~0x20);

	tvp3020xo(0x2C, 0x00);
	tvp3020xo(0x2D, vga->d[0]);
	tvp3020xo(0x2D, vga->n[0]);
	tvp3020xo(0x2D, 0x08|vga->p[0]);

	tvp3020xo(0x2F, 0x01);
	tvp3020xo(0x2F, 0x01);
	tvp3020xo(0x2F, vga->p[0]);
	x = 0x54;
	if(vga->ctlr && (vga->ctlr->flag & Uenhanced))
		x = 0xC4;
	tvp3020xo(0x1E, x);

	vgaxo(Crtx, 0x5C, crt5c);
	vgao(MiscW, vga->misc);

	ctlr->flag |= Fload;
}

static void
tvp3026load(Vga* vga, Ctlr* ctlr)
{
	trace("%s->clock->tvp3026load\n", ctlr->name);

	if((vga->misc & 0x0C) != 0x0C && vga->mode->z == 1){
		tvp3026xo(0x1A, 0x07);
		tvp3026xo(0x18, 0x80);
		tvp3026xo(0x19, 0x98);
		tvp3026xo(0x2C, 0x2A);
		tvp3026xo(0x2F, 0x00);
		tvp3026xo(0x2D, 0x00);
		tvp3026xo(0x39, 0x18);
		setcrt42(vga, ctlr, 0);
	}
	else if(vga->mode->z == 8){
		tvp3026xo(0x1A, 0x05);
		tvp3026xo(0x18, 0x80);
		tvp3026xo(0x19, 0x4C);
		tvp3026xo(0x2C, 0x2A);
		tvp3026xo(0x2F, 0x00);
		tvp3026xo(0x2D, 0x00);

		tvp3026xo(0x2C, 0x00);
		tvp3026xo(0x2D, 0xC0|vga->n[0]);
		tvp3026xo(0x2D, vga->m[0] & 0x3F);
		tvp3026xo(0x2D, 0xB0|vga->p[0]);
		while(!(tvp3026xi(0x2D) & 0x40))
			;

		tvp3026xo(0x39, 0x38|vga->q[1]);
		tvp3026xo(0x2C, 0x00);
		tvp3026xo(0x2F, 0xC0|vga->n[1]);
		tvp3026xo(0x2F, vga->m[1]);
		tvp3026xo(0x2F, 0xF0|vga->p[1]);
		while(!(tvp3026xi(0x2F) & 0x40))
			;

		setcrt42(vga, ctlr, 3);
	}

	ctlr->flag |= Fload;
}

static struct {
	char*	name;
	void	(*load)(Vga*, Ctlr*);
} clocks[] = {
	{ "icd2061a",		icd2061aload, },
	{ "ch9294",		ch9294load, },
	{ "tvp3025clock",	tvp3025load, },
	{ "tvp3026clock",	tvp3026load, },
	{ 0 },
};

static void
init(Vga* vga, Ctlr* ctlr)
{
	char name[Namelen+1], *p;
	int i;

	if(vga->clock == 0)
		return;

	/*
	 * Check we know about it.
	 */
	strncpy(name, vga->clock->name, Namelen);
	name[Namelen] = 0;
	if(p = strchr(name, '-'))
		*p = 0;
	for(i = 0; clocks[i].name; i++){
		if(strcmp(clocks[i].name, name) == 0)
			break;
	}
	if(clocks[i].name == 0)
		error("%s: unknown clock \"%s\"\n", ctlr->name, vga->clock->name);

	if(vga->clock->init && (vga->clock->flag & Finit) == 0)
		(*vga->clock->init)(vga, vga->clock);

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] != VgaFreq0 && vga->f[0] != VgaFreq1)
		vga->misc |= 0x0C;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	char name[Namelen+1], *p;
	int i;

	if(vga->clock == 0 || (vga->clock->flag & Fload))
		return;

	strncpy(name, vga->clock->name, Namelen);
	name[Namelen] = 0;
	if(p = strchr(name, '-'))
		*p = 0;

	for(i = 0; clocks[i].name; i++){
		if(strcmp(clocks[i].name, name))
			continue;
		clocks[i].load(vga, ctlr);
		if(strcmp(clocks[i].name, "icd2061a") == 0){
			clocks[i].load(vga, ctlr);
			clocks[i].load(vga, ctlr);
		}

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
