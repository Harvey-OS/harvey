#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * IBM RGB52x and compatibles.
 * High Performance Palette DAC.
 */
uchar (*rgb524mnxi)(Vga*, int);
void (*rgb524mnxo)(Vga*, int, uchar);

enum {						/* index registers */
	MiscClock	= 0x02,
	SyncControl	= 0x03,
	HSyncControl	= 0x04,
	PowerMgmnt	= 0x05,
	PaletteControl	= 0x07,
	SYSCLKControl	= 0x08,
	PixelFormat	= 0x0A,
	Pixel8Control	= 0x0B,
	Pixel16Control	= 0x0C,
	Pixel32Control	= 0x0E,
	PLLControl1	= 0x10,
	PLLControl2	= 0x11,
	SYSCLKN		= 0x15,
	SYSCLKM		= 0x16,
	M0		= 0x20,
	N0		= 0x21,
	MiscControl1	= 0x70,
	MiscControl2	= 0x71,
};

static void
clock(Vga* vga, Ctlr*, ulong fref, ulong maxpclk)
{
	int d, mind;
	ulong df, f, m, n, vrf;

	mind = vga->f[0]+1;
	for(df = 0; df < 4; df++){
		for(m = 2; m < 64; m++){
			for(n = 2; n < 32; n++){
				f = (fref*(m+65))/n;
				switch(df){
				case 0:
					vrf = fref/(n*2);
					if(vrf > maxpclk/4 || vrf < 1000000)
						continue;
					f /= 8;
					break;
				case 1:
					vrf = fref/(n*2);
					if(vrf > maxpclk/2 || vrf < 1000000)
						continue;
					f /= 4;
					break;
				case 2:
					vrf = fref/(n*2);
					if(vrf > maxpclk || vrf < 1000000)
						continue;
					f /= 2;
					break;
				case 3:
					vrf = fref/n;
					if(vrf > maxpclk || vrf < 1000000)
						continue;
					break;
				}
				if(f > maxpclk)
					continue;

				d = vga->f[0] - f;
				if(d < 0)
					d = -d;
				if(d <= mind){
					vga->m[0] = m;
					vga->n[0] = n;
					vga->d[0] = df;
					mind = d;
				}
			}
		}
	}
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	ulong fref, maxpclk;
	char *p, *val;

	/*
	 * Part comes in at least a -170MHz speed-grade.
	 */
	maxpclk = 170000000;
	if(p = strrchr(ctlr->name, '-'))
		maxpclk = strtoul(p+1, 0, 0) * 1000000;

	/*
	 * If we don't already have a desired pclk,
	 * take it from the mode.
	 * Check it's within range.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	if(vga->f[0] > maxpclk)
		error("%s: invalid pclk - %ld\n", ctlr->name, vga->f[0]);
	if(val = dbattr(vga->attr, "rgb524mnrefclk"))
		fref = strtol(val, 0, 0);
	else
		fref = RefFreq;

	/*
	 * Initialise the PLL parameters.
	 * Use m/n pair 2.
	 */
	clock(vga, ctlr, fref, maxpclk);
	vga->i[0] = 2;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	char *val;
	int hsyncdelay, x;

	if(rgb524mnxi == nil && rgb524mnxo == nil)
		error("%s->load: no access routines\n", ctlr->name);
		
	/*
	 * Set the m/n values for the desired frequency and
	 * set pixel control to use compatibility mode with
	 * internal frequency select using the specified set
	 * of m/n registers.
	 */
	rgb524mnxo(vga, M0+vga->i[0]*2, vga->d[0]<<6|vga->m[0]);
	rgb524mnxo(vga, N0+vga->i[0]*2, vga->n[0]);
	rgb524mnxo(vga, PLLControl2, vga->i[0]);
	rgb524mnxo(vga, PLLControl1, 0x03);

	/*
	 * Enable pixel programming in MiscClock;
	 * nothing to do in MiscControl1;
	 * set internal PLL clock and !vga in MiscControl2;
	 */
	x = rgb524mnxi(vga, MiscClock) & ~0x01;
	x |= 0x01;
	rgb524mnxo(vga, MiscClock, x);

	x = rgb524mnxi(vga, MiscControl2) & ~0x41;
	x |= 0x41;
	rgb524mnxo(vga, MiscControl2, x);

	/*
	 * Syncs.
	 */
	x = 0;
	if(vga->mode->hsync == '+')
		x |= 0x10;
	if(vga->mode->vsync == '+')
		x |= 0x20;
	rgb524mnxo(vga, SyncControl, x);
	if(val = dbattr(vga->mode->attr, "hsyncdelay"))
		hsyncdelay = strtol(val, 0, 0);
	else switch(vga->mode->z){
	default:
	case 8:
		hsyncdelay = 1;
		break;
	case 15:
	case 16:
		hsyncdelay = 5;
		break;
	case 32:
		hsyncdelay = 7;
		break;
	}
	rgb524mnxo(vga, HSyncControl, hsyncdelay);

rgb524mnxo(vga, SYSCLKM, 0x50);
rgb524mnxo(vga, SYSCLKN, 0x08);
sleep(50);
//rgb524mnxo(vga, SYSCLKM, 0x6F);
//rgb524mnxo(vga, SYSCLKN, 0x0F);
//sleep(500);

	/*
	 * Set the palette for the desired format.
	 * ****NEEDS WORK FOR OTHER THAN 8-BITS****
	 */
	rgb524mnxo(vga, PaletteControl, 0x00);
	switch(vga->mode->z){
	case 8:
		rgb524mnxo(vga, PixelFormat, 0x03);
		rgb524mnxo(vga, Pixel8Control, 0x00);
		break;
	case 15:
		rgb524mnxo(vga, PixelFormat, 0x04);
		rgb524mnxo(vga, Pixel16Control, 0xC4);
	case 16:
		rgb524mnxo(vga, PixelFormat, 0x04);
		rgb524mnxo(vga, Pixel16Control, 0xC6);
		break;
	case 32:
		rgb524mnxo(vga, PixelFormat, 0x06);
		rgb524mnxo(vga, Pixel32Control, 0x03);
		break;
	}
}

static void
dumpclock(Vga*, Ctlr* ctlr, ulong fref, ulong m, ulong n, char* name)
{
	ulong df, f;

	df = (m>>6) & 0x03;
	m &= 0x3F;
	n &= 0x1F;
	if(m == 0 || n == 0)
		return;
	f = (fref*(m+65))/n;
	switch(df){
	case 0:
		f /= 8;
		break;
	case 1:
		f /= 4;
		break;
	case 2:
		f /= 2;
		break;
	case 3:
		break;
	}
	printitem(ctlr->name, name);
	Bprint(&stdout, "%12lud\n", f);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *val;
	uchar x[256];
	ulong fref, fs;

	if(rgb524mnxi == nil && rgb524mnxo == nil)
		error("%s->dump: no access routines\n", ctlr->name);

	printitem(ctlr->name, "index00");
	for(i = 0x00; i < 0x0F; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index10");
	for(i = 0x10; i < 0x18; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index20");
	for(i = 0x20; i < 0x30; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index30");
	for(i = 0x30; i < 0x39; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index40");
	for(i = 0x40; i < 0x49; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index60");
	for(i = 0x60; i < 0x63; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index70");
	for(i = 0x70; i < 0x73; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}
	printitem(ctlr->name, "index8E");
	for(i = 0x8E; i < 0x92; i++){
		x[i] = rgb524mnxi(vga, i);
		printreg(x[i]);
	}

	if(val = dbattr(vga->attr, "rgb524mnrefclk"))
		fref = strtol(val, 0, 0);
	else
		fref = RefFreq;
	if(!(x[SYSCLKControl] & 0x04))
		dumpclock(vga, ctlr, fref, x[0x16], x[0x15], "sysclk");
	fs = x[PLLControl1] & 0x07;
	if(fs == 0x01 || fs == 0x03){
		if(fs == 0x01)
			i = ((vga->misc>>2) & 0x03)*2;
		else
			i = x[PLLControl2] & 0x07;
		dumpclock(vga, ctlr, fref, x[M0+i*2], x[N0+i*2], "pllclk");
	}
}

Ctlr rgb524mn = {
	"rgb524mn",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
