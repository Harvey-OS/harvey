#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * 3Dfx.
 */
enum {
	dramInit0		= 0x018/4,
	dramInit1		= 0x01C/4,
	vgaInit0		= 0x028/4,
	pllCtrl0		= 0x040/4,
	pllCtrl1		= 0x044/4,
	pllCtrl2		= 0x048/4,
	dacMode			= 0x04C/4,
	vidProcCfg		= 0x05C/4,
	vidScreenSize		= 0x098/4,
	vidDesktopOverlayStride	= 0x0E8/4,

	Nior			= 0x100/4,
};

typedef struct Tdfx {
	ulong	io;
	Pcidev*	pci;

	ulong	r[Nior];
} Tdfx;

static ulong
io32r(Tdfx* tdfx, int r)
{
	return inportl(tdfx->io+(r*4));
}

static void
io32w(Tdfx* tdfx, int r, ulong l)
{
	outportl(tdfx->io+(r*4), l);
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	ulong v;
	Tdfx *tdfx;

	if(vga->private == nil){
		tdfx = alloc(sizeof(Tdfx));
		tdfx->pci = pcimatch(0, 0x121A, 0);
		if(tdfx->pci == nil)
			error("%s: not found\n", ctlr->name);
		switch(tdfx->pci->did){
		default:
			error("%s: unknown chip - DID %4.4uX\n",
				ctlr->name, tdfx->pci->did);
			break;
		case 0x0003:		/* Banshee */
			vga->f[1] = 270000000;
			break;
		case 0x0005:		/* Avenger (a.k.a. Voodoo3) */
			vga->f[1] = 300000000;
			break;
		case 0x0009:		/* Voodoo5 */
			vga->f[1] = 350000000;
			break;
		}
		/*
		 * Frequency output of PLL's is given by
		 *	fout = RefFreq*(n+2)/((m+2)*2^p)
		 * where there are 6 bits for m, 8 bits for n
		 * and 2 bits for p (k).
		 */
		vga->m[1] = 64;
		vga->n[1] = 256;
		vga->p[1] = 4;

		if((v = (tdfx->pci->mem[2].bar & ~0x3)) == 0)
			error("%s: I/O not mapped\n", ctlr->name);
		tdfx->io = v;

		vga->private = tdfx;
	}
	tdfx = vga->private;

	vga->crt[0x1A] = vgaxi(Crtx, 0x1A);
	vga->crt[0x1B] = vgaxi(Crtx, 0x1B);
	for(i = 0; i < Nior; i++)
		tdfx->r[i] = io32r(tdfx, i);

	/*
	 * If SDRAM then there's 16MB memory else it's SGRAM
	 * and can count it based on the power-on straps -
	 * chip size can be 8Mb or 16Mb, and there can be 4 or
	 * 8 of them.
	 */
	vga->vma = tdfx->pci->mem[1].size;
	if(tdfx->r[dramInit1] & 0x40000000)
		vga->vmz = 16*1024*1024;
	else{
		if(tdfx->r[dramInit0] & 0x08000000)
			i = 16*1024*1024/8;
		else
			i = 8*1024*1024/8;
		if(tdfx->r[dramInit0] & 0x04000000)
			i *= 8;
		else
			i *= 4;
		vga->vmz = i;
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Hclk2|Foptions;
}

static void
tdfxclock(Vga* vga, Ctlr*)
{
	int d, dmin;
	uint f, m, n, p;

	dmin = vga->f[0];
	for(m = 1; m < vga->m[1]; m++){
		for(n = 1; n < vga->n[1]; n++){
			f = (RefFreq*(n+2))/(m+2);
			for(p = 0; p < vga->p[1]; p++){
				d = vga->f[0] - (f/(1<<p));
				if(d < 0)
					d = -d;
				if(d >= dmin)
					continue;
				dmin = d;
				vga->m[0] = m;
				vga->n[0] = n;
				vga->p[0] = p;
			}
		}
	}
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	int x;
	Mode *mode;
	Tdfx *tdfx;

	mode = vga->mode;
	tdfx = vga->private;

	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	/*
	 * Clock bits. If the desired video clock is
	 * one of the two standard VGA clocks or 50MHz it can just be
	 * set using bits <3:2> of vga->misc, otherwise we
	 * need to programme the PLL.
	 */
	if(vga->f[0] == 0)
		vga->f[0] = mode->frequency;
	vga->misc &= ~0x0C;
	if(vga->f[0] == VgaFreq0){
		/* nothing to do */;
	}
	else if(vga->f[0] == VgaFreq1)
		vga->misc |= 0x04;
	else if(vga->f[0] == 50000000)
		vga->misc |= 0x08;
	else{
		if(vga->f[0] > vga->f[1])
			error("%s: invalid pclk - %lud\n",
				ctlr->name, vga->f[0]);
		if(vga->f[0] > 135000000 && (ctlr->flag & Hclk2)){
			if(mode->x%16)
				error("%s: f > 135MHz requires (x%%16) == 0\n",
					ctlr->name);
			ctlr->flag |= Uclk2;
		}
		tdfxclock(vga, ctlr);

		tdfx->r[pllCtrl0] = (vga->n[0]<<8)|(vga->m[0]<<2)|vga->p[0];
		vga->misc |= 0x0C;
	}

	/*
	 * Pixel format and memory stride.
	 */
	tdfx->r[vidScreenSize] = (mode->y<<12)|mode->x;
	tdfx->r[vidProcCfg] = 0x00000081;
	switch(mode->z){
	default:
		error("%s: %d-bit mode not supported\n", ctlr->name, mode->z);
		break;
	case 8:
		tdfx->r[vidDesktopOverlayStride] = mode->x;
		break;
	case 16:
		tdfx->r[vidDesktopOverlayStride] = mode->x*2;
		tdfx->r[vidProcCfg] |= 0x00040400;
		break;
	case 32:
		tdfx->r[vidDesktopOverlayStride] = mode->x*4;
		tdfx->r[vidProcCfg] |= 0x000C0400;
		break;
	}
	tdfx->r[vgaInit0] = 0x140;

	/*
	 * Adjust horizontal timing if doing two screen pixels per clock.
	 */
	tdfx->r[dacMode] = 0;
	if(ctlr->flag & Uclk2){
		vga->crt[0x00] = ((mode->ht/2)>>3)-5;
		vga->crt[0x01] = ((mode->x/2)>>3)-1;
		vga->crt[0x02] = ((mode->shb/2)>>3)-1;
	
		x = (mode->ehb/2)>>3;
		vga->crt[0x03] = 0x80|(x & 0x1F);
		vga->crt[0x04] = (mode->shs/2)>>3;
		vga->crt[0x05] = ((mode->ehs/2)>>3) & 0x1F;
		if(x & 0x20)
			vga->crt[0x05] |= 0x80;

		tdfx->r[dacMode] |= 0x01;
		tdfx->r[vidProcCfg] |= 0x04000000;
	}

	/*
	 * Overflow.
	 */
	vga->crt[0x1A] = 0x00;
	if(vga->crt[0x00] & 0x100)
		vga->crt[0x1A] |= 0x01;
	if(vga->crt[0x01] & 0x100)
		vga->crt[0x1A] |= 0x04;
	if(vga->crt[0x03] & 0x100)
		vga->crt[0x1A] |= 0x10;
	x = mode->ehb;
	if(ctlr->flag & Uclk2)
		x /= 2;
	if((x>>3) & 0x40)
		vga->crt[0x1A] |= 0x20;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x1A] |= 0x40;
	x = mode->ehs;
	if(ctlr->flag & Uclk2)
		x /= 2;
	if((x>>3) & 0x20)
		vga->crt[0x1A] |= 0x80;

	vga->crt[0x1B] = 0x00;
	if(vga->crt[0x06] & 0x400)
		vga->crt[0x1B] |= 0x01;
	if(vga->crt[0x12] & 0x400)
		vga->crt[0x1B] |= 0x04;
	if(vga->crt[0x15] & 0x400)
		vga->crt[0x1B] |= 0x10;
	if(vga->crt[0x10] & 0x400)
		vga->crt[0x1B] |= 0x40;

	vga->attribute[0x11] = Pblack;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	Tdfx *tdfx;

	vgaxo(Crtx, 0x1A, vga->crt[0x1A]);
	vgaxo(Crtx, 0x1B, vga->crt[0x1B]);

	tdfx = vga->private;
	io32w(tdfx, dacMode, tdfx->r[dacMode]);
	io32w(tdfx, vidScreenSize, tdfx->r[vidScreenSize]);
	io32w(tdfx, vidDesktopOverlayStride, tdfx->r[vidDesktopOverlayStride]);
	io32w(tdfx, vidProcCfg, tdfx->r[vidProcCfg]);
	io32w(tdfx, vgaInit0, tdfx->r[vgaInit0]);

	if((vga->misc & 0x0C) == 0x0C)
		io32w(tdfx, pllCtrl0, tdfx->r[pllCtrl0]);

	ctlr->flag |= Fload;
}

static uint
pllctrl(Tdfx* tdfx, int pll)
{
	uint k, m, n, r;

	r = tdfx->r[pllCtrl0+pll];
	k = r & 0x03;
	m = (r>>2) & 0x3F;
	n = (r>>8) & 0xFF;

	return (RefFreq*(n+2))/((m+2)*(1<<k));
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	Tdfx *tdfx;

	if((tdfx = vga->private) == nil)
		return;

	printitem(ctlr->name, "Crt1A");
	printreg(vga->crt[0x1A]);
	printreg(vga->crt[0x1B]);

	Bprint(&stdout, "\n");
	for(i = 0; i < Nior; i++)
		Bprint(&stdout, "%s %2.2uX\t%.8luX\n",
			ctlr->name, i*4, tdfx->r[i]);

	printitem(ctlr->name, "pllCtrl");
	Bprint(&stdout, "%9ud %8ud\n", pllctrl(tdfx, 0), pllctrl(tdfx, 1));
}

Ctlr tdfx = {
	"3dfx",				/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr tdfxhwgc = {
	"3dfxhwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
