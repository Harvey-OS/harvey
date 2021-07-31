#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"
#include "riva_tbl.h"

typedef struct Nvidia	Nvidia;
struct Nvidia {
	ulong	mmio;
	Pcidev*	pci;

	int		arch;
	int		crystalfreq;

	ulong*	pfb;			/* mmio pointers */
	ulong*	pramdac;
	ulong*	pextdev;
	ulong*	pmc;
	ulong*	ptimer;
	ulong*	pfifo;
	ulong*	pramin;
	ulong*	pgraph;
	ulong*	fifo;

	ulong	cursor2;
	ulong	vpll;
	ulong	pllsel;
	ulong	general;
	ulong	config;

	ulong	offset[4];
	ulong	pitch[5];
};

static int extcrts[] = {
	0x19, 0x1A, 0x25, 0x28, 0x2D, 0x30, 0x31, -1
};

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	Pcidev *p;
	ulong m, *mmio, tmp;
	int i;

	if(vga->private == nil){
		vga->private = alloc(sizeof(Nvidia));
		nv = vga->private;
		if((p = pcimatch(0, 0x10DE, 0)) == nil)
			error("%s: not found\n", ctlr->name);
		switch(p->did){
		case 0x0020:		/* Riva TNT */
		case 0x0028:		/* Riva TNT2 */
		case 0x0029:		/* Riva TNT2 (Ultra)*/
		case 0x002C:		/* Riva TNT2 (Vanta) */
		case 0x002D:		/* Riva TNT2 M64 */
		case 0x00A0:		/* Riva TNT2 (Integrated) */
			nv->arch = 4;
			break;
		case 0x0100:		/* GeForce 256 */
		case 0x0101:		/* GeForce DDR */
		case 0x0103:		/* Quadro */
		case 0x0110:		/* GeForce2 MX */
		case 0x0111:		/* GeForce2 MX DDR */
		case 0x0112:		/* GeForce 2 Go */
		case 0x0113:		/* Quadro 2 MXR */
		case 0x0150:		/* GeForce2 GTS */
		case 0x0151:		/* GeForce2 GTS (rev 1) */
		case 0x0152:		/* GeForce2 Ultra */
		case 0x0153:		/* Quadro 2 Pro */
		case 0x0200:		/* GeForce3 */
			nv->arch = 10;
			break;
		default:
			error("%s: DID %4.4uX unsupported\n",
				ctlr->name, p->did);
		}

		vgactlw("type", ctlr->name);

		if((m = segattach(0, "nvidiammio", 0, p->mem[0].size)) == -1)
			error("%s: can't attach mmio segment\n", ctlr->name);

		nv->pci = p;
		nv->mmio = m;

		mmio = (ulong*)m;
		nv->pfb = mmio+0x00100000/4;
		nv->pramdac = mmio+0x00680000/4;
		nv->pextdev = mmio+0x00101000/4;
		nv->pmc	= mmio+0x00000000/4;
		nv->ptimer = mmio+0x00009000/4;
		nv->pfifo = mmio+0x00002000/4;
		nv->pramin = mmio+0x00710000/4;
		nv->pgraph = mmio+0x00400000/4;
		nv->fifo = mmio+0x00800000/4;
	}
	nv = vga->private;

	vga->p[1] = 4;
	vga->n[1] = 255;
	vga->f[1] = 350000000;

	/*
	 * Unlock
	 */
	vgaxo(Crtx, 0x1F, 0x57);

	if (nv->pextdev[0x00000000] & 0x00000040){
		nv->crystalfreq = RefFreq;
		vga->m[1] = 14;
	} else {
		nv->crystalfreq = 13500000;
		vga->m[1] = 13;
	}

	if (nv->arch == 4) {
		tmp = nv->pfb[0x00000000];
		if (tmp & 0x0100) {
			vga->vmz = ((tmp >> 12) & 0x0F) * 1024 + 1024 * 2;
		} else {
			tmp &= 0x03;
			if (tmp)
				vga->vmz = (1024*1024*2) << tmp;
			else
				vga->vmz = 1024*1024*32;
		}
	}
	if (nv->arch == 10) {
		tmp = (nv->pfb[0x0000020C/4] >> 20) & 0xFF;
		if (tmp == 0)
			tmp = 16;
		vga->vmz = 1024*1024*tmp;
	}

	for(i=0; i<4; i++)
		nv->offset[i] = nv->pgraph[0x640/4+i];
	for(i=0; i<5; i++)
		nv->pitch[i] = nv->pgraph[0x670/4+i];

	for(i = 0x19; i < 0x100; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	nv->cursor2	= nv->pramdac[0x00000300/4];
	nv->vpll	= nv->pramdac[0x00000508/4];
	nv->pllsel	= nv->pramdac[0x0000050C/4];
	nv->general	= nv->pramdac[0x00000600/4];
	nv->config	= nv->pfb[0x00000200/4];


	ctlr->flag |= Fsnarf;
}


static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Foptions;
}


static void
clock(Vga* vga, Ctlr* ctlr)
{
	int m, n, p, f, d;
	Nvidia *nv;
	double trouble;

	nv = vga->private;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	vga->d[0] = vga->f[0]+1;

	for (p=0; p <= vga->p[1]; p++){
		f = vga->f[0] << p;
		if ((f >= 128000000) && (f <= vga->f[1])) {
			for (m=7; m <= vga->m[1]; m++){
				trouble = (double) nv->crystalfreq / (double) (m << p);
				n = (vga->f[0] / trouble)+0.5;
				f = n*trouble + 0.5;
				d = vga->f[0] - f;
				if (d < 0)
					d = -d;
				if (n & ~0xFF)
					d = vga->d[0] + 1;
				if (d <= vga->d[0]){
					vga->n[0] = n;
					vga->m[0] = m;
					vga->p[0] = p;
					vga->d[0] = d;
				}
			}
		}
	}
	if (vga->d[0] > vga->f[0])
		error("%s: vclk %lud out of range\n", ctlr->name, vga->f[0]);
}


static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	Nvidia *nv;
	int digital, i, tmp;
	int pixeldepth;

	mode = vga->mode;
	if(mode->z == 24)
		error("%s: 24-bit colour not supported, use 32-bit\n", ctlr->name);

	nv = vga->private;

	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	clock(vga, ctlr);

	digital = vga->crt[0x28] & 0x80;
	for(i = 0; extcrts[i] >= 0; i++)
		vga->crt[extcrts[i]] = 0;

	vga->crt[0x30] = 0x00;
	vga->crt[0x31] = 0xFC;
	nv->cursor2 = 0;
	nv->vpll    = (vga->p[0] << 16) | (vga->n[0] << 8) | vga->m[0];
	nv->pllsel  = 0x10000700;
	if (mode->z == 16)
		nv->general = 0x00001100;
	else
		nv->general = 0x00000100;
	if(nv->arch != 10)
		nv->config  = 0x00001114;

	pixeldepth = (mode->z +1)/8;
	tmp = pixeldepth * mode->x;
	nv->pitch[0] = tmp;
	nv->pitch[1] = tmp;
	nv->pitch[2] = tmp;
	nv->pitch[3] = tmp;
	nv->pitch[4] = tmp;
	nv->offset[0] = 0;
	nv->offset[1] = 0;
	nv->offset[2] = 0;
	nv->offset[3] = 0;

	vga->attribute[0x10] &= ~0x40;
	vga->attribute[0x11] = Pblack;
	vga->crt[0x14] = 0x00;

	/* set vert blanking to cover full overscan */

	tmp = vga->crt[0x12];
	vga->crt[0x15] = tmp;
	if(tmp & 0x100)
		vga->crt[0x07] |= 0x08;
	else
		vga->crt[0x07] &= ~0x08;
	if(tmp & 0x200)
		vga->crt[0x09] |= 0x20;
	else
		vga->crt[0x09] &= ~0x20;

	vga->crt[0x16] = vga->crt[0x06] + 1;

	/* set horiz blanking to cover full overscan */

	vga->crt[0x02] = vga->crt[0x01];
	tmp = vga->crt[0x00] + 4;
	vga->crt[0x03] = 0x80 | (tmp & 0x1F);
	if (tmp & 0x20)
		vga->crt[0x05] |= 0x80;
	else
		vga->crt[0x05] &= ~0x80;
	if (tmp & 0x40)
		vga->crt[0x25] |= 0x10;

	/* overflow bits */

	vga->crt[0x1A] = 0x02;
	if (mode->x < 1280)
		vga->crt[0x1A] |= 0x04;

	tmp = pixeldepth;
	if (tmp > 3)
		tmp=3;
	vga->crt[0x28] = digital|tmp;

	vga->crt[0x19] = (vga->crt[0x13] & 0x0700) >> 3;

	if (vga->crt[0x06] & 0x400)
		vga->crt[0x25] |= 0x01;
	if (vga->crt[0x12] & 0x400)
		vga->crt[0x25] |= 0x02;
	if (vga->crt[0x10] & 0x400)
		vga->crt[0x25] |= 0x04;
	if (vga->crt[0x15] & 0x400)
		vga->crt[0x25] |= 0x08;
	if (vga->crt[0x13] & 0x800)
		vga->crt[0x25] |= 0x20;

	if (vga->crt[0x00] & 0x100)
		vga->crt[0x2D] |= 0x01;
	if(vga->crt[0x01] & 0x100)
		vga->crt[0x2D] |= 0x02;
	if(vga->crt[0x02] & 0x100)
		vga->crt[0x2D] |= 0x04;
	if(vga->crt[0x04] & 0x100)
		vga->crt[0x2D] |= 0x08;

	ctlr->flag |= Finit;
}

#define loadtable(p, tab) _loadtable(p, tab, nelem(tab))

static void
_loadtable(ulong *p, uint tab[][2], int ntab)
{
	int i;

	for(i=0; i<ntab; i++)
		p[tab[i][0]] = tab[i][1];
}

static struct {
	ulong addr;
	ulong val;
	int rep;
} gtab[] = {
	{ 0x0F40, 0x10000000, 1 },
	{ 0x0F44, 0x00000000, 1 },
	{ 0x0F50, 0x00000040, 1 },
	{ 0x0F54, 0x00000008, 1 },
	{ 0x0F50, 0x00000200, 1 },
	{ 0x0F54, 0x00000000, 3*16 },
	{ 0x0F50, 0x00000040, 1 },
	{ 0x0F54, 0x00000000, 1 },
	{ 0x0F50, 0x00000800, 1 },
	{ 0x0F54, 0x00000000, 16*16 },
	{ 0x0F40, 0x30000000, 1 },
	{ 0x0F44, 0x00000004, 1 },
	{ 0x0F50, 0x00006400, 1 },
	{ 0x0F54, 0x00000000, 59*4 },
	{ 0x0F50, 0x00006800, 1 },
	{ 0x0F54, 0x00000000, 47*4 },
	{ 0x0F50, 0x00006C00, 1 },
	{ 0x0F54, 0x00000000, 3*4 },
	{ 0x0F50, 0x00007000, 1 },
	{ 0x0F54, 0x00000000, 19*4 },
	{ 0x0F50, 0x00007400, 1 },
	{ 0x0F54, 0x00000000, 12*4 },
	{ 0x0F50, 0x00007800, 1 },
	{ 0x0F54, 0x00000000, 12*4 },
	{ 0x0F50, 0x00004400, 1 },
	{ 0x0F54, 0x00000000, 8*4 },
	{ 0x0F50, 0x00000000, 1 },
	{ 0x0F54, 0x00000000, 16 },
	{ 0x0F50, 0x00000040, 1 },
	{ 0x0F54, 0x00000000, 4 },
};

static void
load(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	int i, j;

	nv = vga->private;

	/*
	 * Unlock
	 */
	vgaxo(Crtx, 0x1F, 0x57);

	loadtable(nv->pmc, RivaTablePMC);
	loadtable(nv->ptimer, RivaTablePTIMER);
	switch (nv->arch){
	case 4:
		nv->pfb[0x00000200/4] = nv->config;
		loadtable(nv->pfifo, nv4TablePFIFO);
		loadtable(nv->pramin, nv4TablePRAMIN);
		loadtable(nv->pgraph, nv4TablePGRAPH);
		switch (vga->mode->z){
		case 32:
			loadtable(nv->pramin, nv4TablePRAMIN_32BPP);
			loadtable(nv->pgraph, nv4TablePGRAPH_32BPP);
			break;
		case 16:
			loadtable(nv->pramin, nv4TablePRAMIN_16BPP);
			loadtable(nv->pgraph, nv4TablePGRAPH_16BPP);
			break;
		case 8:
			loadtable(nv->pramin, nv4TablePRAMIN_8BPP);
			loadtable(nv->pgraph, nv4TablePGRAPH_8BPP);
			break;
		}
		break;
	case 10:
		loadtable(nv->pfifo, nv10TablePFIFO);
		loadtable(nv->pramin, nv10TablePRAMIN);
		loadtable(nv->pgraph, nv10TablePGRAPH);
		switch (vga->mode->z){
		case 32:
			loadtable(nv->pramin, nv10TablePRAMIN_32BPP);
			loadtable(nv->pgraph, nv10TablePGRAPH_32BPP);
			break;
		case 16:
			loadtable(nv->pramin, nv10TablePRAMIN_16BPP);
			loadtable(nv->pgraph, nv10TablePGRAPH_16BPP);
			break;
		case 8:
			loadtable(nv->pramin, nv10TablePRAMIN_8BPP);
			loadtable(nv->pgraph, nv10TablePGRAPH_8BPP);
			break;
		}
		break;
	}

	for(i=0; i<4; i++)
		nv->pgraph[0x640/4+i] = nv->offset[i];
	if(nv->arch == 10)
		j = 5;
	else
		j = 4;
	for(i=0; i<j; i++)
		nv->pgraph[0x670/4+i] = nv->pitch[i];

	nv->pmc[0x8704/4] = 1;
	nv->pmc[0x8140/4] = 0;
	nv->pmc[0x8920/4] = 0;
	nv->pmc[0x8924/4] = 0;
	nv->pmc[0x8908/4] = 0x01FFFFFF;
	nv->pmc[0x890C/4] = 0x01FFFFFF;

	if(nv->arch == 10){
		for(i=0; i<0x7C/4; i++)
			nv->pgraph[0xB00/4+i] = nv->pfb[0x240/4+i];

		for(i=0; i<nelem(gtab); i++)
			for(j=0; j<gtab[i].rep; j++)
				nv->pgraph[gtab[i].addr/4] = gtab[i].val;
	}

	loadtable(nv->fifo, RivaTableFIFO);

	switch (nv->arch){
	case 4:
		loadtable(nv->fifo, nv4TableFIFO);
		break;
	case 10:
		loadtable(nv->fifo, nv10TableFIFO);
		break;
	}

	for(i = 0; extcrts[i] >= 0; i++)
		vgaxo(Crtx, extcrts[i], vga->crt[extcrts[i]]);

	nv->pramdac[0x00000300/4] = nv->cursor2;
	nv->pramdac[0x00000508/4] = nv->vpll;
	nv->pramdac[0x0000050C/4] = nv->pllsel;
	nv->pramdac[0x00000600/4] = nv->general;

	ctlr->flag |= Fload;
}


static void
dump(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	int i, m, n, p, f;
	double trouble;

	if((nv = vga->private) == 0)
		return;

	printitem(ctlr->name, "Crt18");
	for(i = 0x18; i < 0x100; i++)
		printreg(vga->crt[i]);

	p = (nv->vpll >> 16);
	n = (nv->vpll >> 8) & 0xFF;
	m = nv->vpll & 0xFF;
	trouble = nv->crystalfreq;
	trouble = trouble * n / (m<<p);
	f = trouble+0.5;
	printitem(ctlr->name, "dclk m n p");
	Bprint(&stdout, " %d %d - %d %d\n", f, m, n, p);
	printitem(ctlr->name, "CrystalFreq");
	Bprint(&stdout, " %d Hz\n", nv->crystalfreq);
	printitem(ctlr->name, "cursor2");
	Bprint(&stdout, " %lux\n", nv->cursor2);
	printitem(ctlr->name, "vpll");
	Bprint(&stdout, " %lux\n", nv->vpll);
	printitem(ctlr->name, "pllsel");
	Bprint(&stdout, " %lux\n", nv->pllsel);
	printitem(ctlr->name, "general");
	Bprint(&stdout, " %lux\n", nv->general);
	printitem(ctlr->name, "config");
	Bprint(&stdout, " %lux\n", nv->config);
}

Ctlr nvidia = {
	"nvidia",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr nvidiahwgc = {
	"nvidiahwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};

