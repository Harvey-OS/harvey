#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * IBM RGB524.
 * 170/220MHz High Performance Palette DAC.
 *
 * Assumes hooked up to an S3 Vision96[48].
 */
enum {
	IndexLo		= 0x00,
	IndexHi		= 0x01,
	Data		= 0x02,
	IndexCtl	= 0x03,
};

enum {						/* index registers */
	MiscClock	= 0x02,
	PixelFormat	= 0x0A,
	PLLControl1	= 0x10,
	PLLControl2	= 0x11,
	PLLReference	= 0x14,
	Frequency0	= 0x20,
	MiscControl1	= 0x70,
	MiscControl2	= 0x71,
};

static uchar
setrs2(void)
{
	uchar rs2;

	rs2 = vgaxi(Crtx, 0x55);
	vgaxo(Crtx, 0x55, (rs2 & 0xFC)|0x01);

	return rs2;
}

static uchar
rgb524xi(int index)
{
	outportb(dacxreg[IndexLo], index & 0xFF);
	outportb(dacxreg[IndexHi], (index>>8) & 0xFF);

	return inportb(dacxreg[Data]);
}

static void
rgb524xo(int index, uchar data)
{
	outportb(dacxreg[IndexLo], index & 0xFF);
	outportb(dacxreg[IndexHi], (index>>8) & 0xFF);

	outportb(dacxreg[Data], data);
}

static void
restorers2(uchar rs2)
{
	vgaxo(Crtx, 0x55, rs2);
}

static void
clock(Vga* vga, Ctlr* ctlr)
{
	if(vga->f[0] >= 16250000 && vga->f[0] <= 32000000){
		vga->f[0] = (vga->f[0]/250000)*250000;
		vga->d[0] = (4*vga->f[0])/1000000 - 65;
	}
	else if(vga->f[0] >= 32500000 && vga->f[0] <= 64000000){
		vga->f[0] = (vga->f[0]/500000)*500000;
		vga->d[0] = 0x40|((2*vga->f[0])/1000000 - 65);
	}
	else if(vga->f[0] >= 65000000 && vga->f[0] <= 128000000){
		vga->f[0] = (vga->f[0]/1000000)*1000000;
		vga->d[0] = 0x80|(vga->f[0]/1000000 - 65);
	}
	else if(vga->f[0] >= 130000000 && vga->f[0] <= 220000000){
		vga->f[0] = (vga->f[0]/2000000)*2000000;
		vga->d[0] = 0xC0|((vga->f[0]/2)/1000000 - 65);
	}
	else
		error("%s: pclk %lud out of range\n",
			ctlr->name, vga->f[0]);
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong pclk;
	char *p;

	/*
	 * Part comes in -170 and -220MHz speed-grades.
	 */
	pclk = 170000000;
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

	/*
	 * Determine whether to use clock-doubler or not.
	 */
	if((ctlr->flag & Uclk2) == 0 && vga->mode->z == 8)
		resyncinit(vga, ctlr, Uclk2, 0);

	/*
	 * Clock bits. If the desired video clock is
	 * one of the two standard VGA clocks it can just be
	 * set using bits <3:2> of vga->misc, otherwise we
	 * need to programme the PLL.
	 */
	vga->misc &= ~0x0C;
	if(vga->mode->z == 8 || (vga->f[0] != VgaFreq0 && vga->f[0] != VgaFreq1)){
		/*
		 * Initialise the PLL parameters.
		 * Use internal FS3 fixed-reference divider.
		 */
		clock(vga, ctlr);
		vga->i[0] = 0x03;
	}
	else if(vga->f[0] == VgaFreq0)
		vga->i[0] = 0;
	else if(vga->f[0] == VgaFreq1){
		vga->misc |= 0x04;
		vga->i[0] = 1;
	}

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	uchar mc2, rs2, x;
	char *val;
	int f;

	rs2 = setrs2();

	/*
	 * Set VgaFreq[01].
	 */
	rgb524xo(PLLControl1, 0x00);
	rgb524xo(Frequency0, 0x24);
	rgb524xo(Frequency0+1, 0x30);

	if(val = dbattr(vga->attr, "rgb524refclk")){
		f = strtol(val, 0, 0);
		if(f > 1000000)
			f /= 1000000;
		rgb524xo(PLLReference, f/2);
	}
		
	/*
	 * Enable pixel programming and clock divide
	 * factor.
	 */
	x = rgb524xi(MiscClock) & ~0x0E;
	x |= 0x01;
	if(ctlr->flag & Uclk2)
		x |= 0x02;
	rgb524xo(MiscClock, x);

	if(vga->mode->z == 1)
		rgb524xo(PixelFormat, 0x02);
	else if(vga->mode->z == 8)
		rgb524xo(PixelFormat, 0x03);

	x = rgb524xi(MiscControl1) & ~0x41;
	x |= 0x01;
	rgb524xo(MiscControl1, x);

	mc2 = rgb524xi(MiscControl2) & ~0x41;
	vga->crt[0x22] &= ~0x08;
	if(vga->i[0] == 3){
		rgb524xo(Frequency0+3, vga->d[0]);
		rgb524xo(PLLControl1, 0x02);
		rgb524xo(PLLControl2, vga->i[0]);
		mc2 |= 0x41;
		vga->crt[0x22] |= 0x08;
	}
	rgb524xo(MiscControl2, mc2);
	vgaxo(Crtx, 0x22, vga->crt[0x22]);

	restorers2(rs2);
	ctlr->flag |= Fload;
}

static void
dump(Vga*, Ctlr* ctlr)
{
	uchar rs2, r, x[256];
	char buf[32];
	int df, i, maxf, vcodc, vf;

	rs2 = setrs2();
	printitem(ctlr->name, "index00");
	for(i = 0x00; i < 0x0F; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index10");
	for(i = 0x10; i < 0x17; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index20");
	for(i = 0x20; i < 0x30; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index30");
	for(i = 0x30; i < 0x37; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index40");
	for(i = 0x40; i < 0x49; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index60");
	for(i = 0x60; i < 0x63; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index70");
	for(i = 0x70; i < 0x73; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index8E");
	for(i = 0x8E; i < 0x92; i++){
		x[i] = rgb524xi(i);
		printreg(x[i]);
	}
	restorers2(rs2);

	/*
	 * x[0x10]	pixel clock frequency selection
	 *		0, 2 for direct programming
	 * x[0x20-0x2F]	pixel frequency 0-15
	 */
	printitem(ctlr->name, "refclk");
	Bprint(&stdout, "%12ud\n", x[PLLReference]*2*1000000);
	if((i = (x[0x10] & 0x07)) == 0x00 || i == 0x02){
		/*
		 * Direct programming, external frequency select.
		 * F[0-4] are probably tied directly to the 2 clock-select
		 * bits in the VGA Misc register.
		 */
		if(i == 0)
			maxf = 4;
		else
			maxf = 16;
		for(i = 0; i < maxf; i++){
			if((r = x[0x20+i]) == 0)
				continue;
			sprint(buf, "direct F%X", i);
			printitem(ctlr->name, buf);
			df = (r>>6) & 0x03;
			vcodc = r & 0x3F;

			vf = 0;
			switch(df){
			case 0:
				vf = (vcodc+65)/4;
				break;

			case 1:
				vf = (vcodc+65)/2;
				break;

			case 2:
				vf = (vcodc+65);
				break;

			case 3:
				vf = (vcodc+65)*2;
				break;
			}
			Bprint(&stdout, "%12ud\n", vf);
		}
	}
}

Ctlr rgb524 = {
	"rgb524",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
