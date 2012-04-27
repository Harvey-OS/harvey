#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Matrox Millenium and Matrox Millenium II.
 * Matrox MGA-2064W, MGA-2164W 3D graphics accelerators
 * Texas Instruments Tvp3026 RAMDAC.
 */
enum {
	Meg		= 1024*1024,
	/* pci chip manufacturer */
	MATROX		= 0x102B,

	/* pci chip device ids */
	MGA2064		= 0x0519,
	MGA2164		= 0x051B,
	MGA2164AGP	= 0x051F,

	/* i/o ports */
	Crtcext		= 0x03DE,	/* CRTC Extensions */

	/* config space offsets */
	Devctrl		= 0x04,		/* Device Control */
	Option		= 0x40,		/* Option */

	/* control aperture offsets */
	RAMDAC		= 0x3C00,	/* RAMDAC registers */
	CACHEFLUSH	= 0x1FFF,
};

typedef struct {
	Pcidev*	pci;
	int	devid;
	uchar*	membase1;
	uchar*	membase2;
	ulong	devctrl;
	ulong	option;
	uchar	crtcext[6];
	uchar	tvp[64];
	uchar	pclk[3];
	uchar	mclk[3];
	uchar	lclk[3];
} Mga;

static uchar
_tvp3026i(Vga* vga, Ctlr* ctlr, uchar reg)
{
	Mga *mga;

	if(reg >= 0x10)
		error("%s: tvp3026io: direct reg 0x%uX out of range\n", ctlr->name, reg);

	if(vga->private == nil)
		error("%s: tvp3026io: no *mga\n", ctlr->name);
	mga = vga->private;

	return *(mga->membase1+RAMDAC+reg);
}

static void
_tvp3026o(Vga* vga, Ctlr* ctlr, uchar reg, uchar data)
{
	Mga *mga;

	if(reg >= 0x10)
		error("%s: tvp3026io: direct reg 0x%uX out of range\n", ctlr->name, reg);

	if(vga->private == nil)
		error("%s: tvp3026io: no *mga\n", ctlr->name);
	mga = vga->private;

	*(mga->membase1+RAMDAC+reg) = data;
}

static uchar
_tvp3026xi(Vga* vga, Ctlr* ctlr, uchar index)
{
	if(index >= 0x40)
		error("%s: tvp3026xi: reg 0x%uX out of range\n", ctlr->name, index);

	_tvp3026o(vga, ctlr, 0x00, index);

	return _tvp3026i(vga, ctlr, 0x0A);
}

void
_tvp3026xo(Vga* vga, Ctlr* ctlr, uchar index, uchar data)
{
	if(index >= 0x40)
		error("%s: tvp3026xo: reg 0x%uX out of range\n", ctlr->name, index);

	_tvp3026o(vga, ctlr, 0x00, index);
	_tvp3026o(vga, ctlr, 0x0A, data);
}

static uchar
crtcexti(uchar index)
{
	uchar data;

	outportb(Crtcext, index);
	data = inportb(Crtcext+1);

	return data;
}

static void
crtcexto(uchar index, uchar data)
{
	outportb(Crtcext, index);
	outportb(Crtcext+1, data);
}

static void
mapmga(Vga* vga, Ctlr* ctlr)
{
	int f;
	uchar *m;
	Mga *mga;

	if(vga->private == nil)
		error("%s: tvp3026io: no *mga\n", ctlr->name);
	mga = vga->private;

	f = open("#v/vgactl", OWRITE);
	if(f < 0)
		error("%s: can't open vgactl\n", ctlr->name);
	if(write(f, "type mga2164w", 13) != 13)
		error("%s: can't set mga type\n", ctlr->name);
	
	m = segattach(0, "mga2164wmmio", 0, 16*1024);
	if(m == (void*)-1)
		error("%s: can't attach mga2164wmmio segment\n", ctlr->name);
	mga->membase1 = m;

	m = segattach(0, "mga2164wscreen", 0, (mga->devid==MGA2064? 8 : 16)*Meg);
	if(m ==(void*)-1)
		error("%s: can't attach mga2164wscreen segment\n", ctlr->name);

	mga->membase2 = m;
}

static void
clockcalc(Vga* vga, Ctlr* ctlr, int bpp)
{
	ulong m, n, p, q;
	double z, fdiff, fmindiff, fvco, fgoal;
	Mga *mga;

	USED(ctlr);

	mga = vga->private;

	/*
	 * Look for values of n, d and p that give
	 * the least error for
	 *	Fvco = 8*RefFreq*(65-m)/(65-n)
	 *	Fpll = Fvco/2**p
	 * N and m are 6 bits, p is 2 bits. Constraints:
	 *	110MHz <= Fvco <= 250MHz
	 *	40 <= n <= 62
	 *	 1 <= m <= 62
	 *	 0 <= p <=  3
	 */
	fgoal = (double)vga->f[0];
	fmindiff = fgoal;
	vga->m[0] = 0x15;
	vga->n[0] = 0x18;
	vga->p[0] = 3;
	for(m = 62; m > 0; m--){
		for(n = 62; n >= 40; n--){
			fvco = 8.*((double)RefFreq)*(65.-m)/(65.-n);
			if(fvco < 110e6 || fvco > 250e6)
				continue;
			for(p = 0; p < 4; p++){
				fdiff = fgoal - fvco/(1<<p);
				if(fdiff < 0)
					fdiff = -fdiff;
				if(fdiff < fmindiff){
					fmindiff = fdiff;
					vga->m[0] = m;
					vga->n[0] = n;
					vga->p[0] = p;
				}
			}
		}
	}
	mga->pclk[0] = 0xC0 | (vga->n[0] & 0x3f);
	mga->pclk[1] = (vga->m[0] & 0x3f);
	mga->pclk[2] = 0xB0 | (vga->p[0] & 0x03);

	/*
	 * Now the loop clock:
	 * 	m is fixed;
	 *	calculate n;
	 *	set z to the lower bound (110MHz) and calculate p and q.
	 */
	vga->m[1] = 61;
	n = 65 - 4*(64/(vga->vmz==2*Meg? bpp*2 : bpp));
	fvco = (8*RefFreq*(65-vga->m[0]))/(65-vga->n[0]);
	vga->f[1] = fvco/(1<<vga->p[0]);
	z = 110e6*(65.-n)/(4.*vga->f[1]);
	if(z <= 16){
		for(p = 0; p < 4; p++){
			if(1<<(p+1) > z)
				break;
		}
		q = 0;
	}
	else{
		p = 3;
		q = (z - 16)/16 + 1;
	}
	vga->n[1] = n;
	vga->p[1] = p;
	vga->q[1] = q;
	mga->lclk[0] = 0xC0 | (vga->n[1] & 0x3f);
	mga->lclk[1] = (vga->m[1] & 0x3f);
	mga->lclk[2] = 0xF0 | (vga->p[1] & 0x03);
	mga->tvp[0x39] = 0x38 | q;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i, k, n;
	uchar *p, x[8];
	Pcidev *pci;
	Mga *mga;

	if(vga->private == nil) {
		pci = pcimatch(nil, MATROX, MGA2164AGP);
		if(pci == nil)
			pci = pcimatch(nil, MATROX, MGA2164);
		if(pci == nil)
			pci = pcimatch(nil, MATROX, MGA2064);
		if(pci == nil)
			error("%s: no Pcidev with Vid=0x102B, Did=0x051[9BF] or 0x521\n", ctlr->name);

		vga->private = alloc(sizeof(Mga));
		mga = (Mga*)vga->private;
		mga->devid = pci->did;
		mga->pci = pci;
		mapmga(vga, ctlr);
	}
	else {
		mga = (Mga*)vga->private;
		pci = mga->pci;
	}

	for(i = 0; i < 0x06; i++)
		mga->crtcext[i] = crtcexti(i);

	mga->devctrl = pcicfgr32(pci, Devctrl);
	mga->option = pcicfgr32(pci, Option);

	for(i = 0; i < 64; i++)
		mga->tvp[i] = _tvp3026xi(vga, ctlr, i);

	for(i = 0; i < 3; i++){
		_tvp3026xo(vga, ctlr, 0x2C, (i<<4)|(i<<2)|i);
		mga->pclk[i] = _tvp3026xi(vga, ctlr, 0x2D);
	}

	for(i = 0; i < 3; i++){
		_tvp3026xo(vga, ctlr, 0x2C, (i<<4)|(i<<2)|i);
		mga->mclk[i] = _tvp3026xi(vga, ctlr, 0x2E);
	}

	for(i = 0; i < 3; i++){
		_tvp3026xo(vga, ctlr, 0x2C, (i<<4)|(i<<2)|i);
		mga->lclk[i] = _tvp3026xi(vga, ctlr, 0x2F);
	}

	/* find out how much memory is here, some multiple of 2Meg */
	crtcexto(3, mga->crtcext[3] | 0x80);	/* mga mode */
	p = mga->membase2;
	n = (mga->devid==MGA2064? 4 : 8);
	for(i = 0; i < n; i++) {
		k = (2*i+1)*Meg;
		p[k] = 0;
		p[k] = i+1;
		*(mga->membase1+CACHEFLUSH) = 0;
		x[i] = p[k];
		trace("x[%d]=%d\n", i, x[i]);
	}
	for(i = 1; i < n; i++)
		if(x[i] != i+1)
			break;
	vga->vmz = 2*i*Meg;
	trace("probe found %d megabytes\n", 2*i);
	crtcexto(3, mga->crtcext[3]);	/* restore mga mode */


	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	if(vga->virtx & 127)
		vga->virtx = (vga->virtx+127)&~127;
	ctlr->flag |= Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	Ctlr *c;
	Mga *mga;
	int scale, offset, pixbuswidth;

	mga = vga->private;
	mode = vga->mode;

	ctlr->flag |= Ulinear;

	pixbuswidth = (vga->vmz == 2*Meg) ? 32 : 64;
	trace("pixbuswidth=%d\n", pixbuswidth);

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	/* supposed to let tvp reg 1D control sync polarity */
	vga->misc |= 0xC8;

	/*
	 * In power graphics mode, supposed to use
	 *	hblkend = htotal+4
	 *	hblkstr = hdispend
	 */
	if((vga->crt[0x00] & 0x0F) == 0x0F) {
		vga->crt[0x00]++;
		mode->ht += 8;
	}
	vga->crt[0x02] = vga->crt[0x01];
	vga->crt[0x03] = (((mode->ht>>3)-1) & 0x1F) | 0x80;
	vga->crt[0x05] = ((((mode->ht>>3)-1) & 0x20) << 2)
		| ((mode->ehs>>3) & 0x1F);

	offset = (vga->virtx*mode->z) >> ((pixbuswidth==32)? 6 : 7);
	vga->crt[0x13] = offset;
	vga->crt[0x14] = 0;
	vga->crt[0x17] = 0xE3;

	mga->crtcext[0] = (offset & 0x300) >> 4;
	mga->crtcext[1] = ((((mode->ht>>3)-5) & 0x100) >> 8)
		| ((((mode->x>>3)-1) & 0x100) >> 7)
		| (((mode->shs>>3) & 0x100) >> 6)  /* why not (shs>>3)-1 ? */
		| (((mode->ht>>3)-1) & 0x40);
	mga->crtcext[2] = (((mode->vt-2) & 0xC00) >> 10)
		| (((mode->y-1) & 0x400) >> 8)
		| ((mode->vrs & 0xC00) >> 7)
		| ((mode->vrs & 0xC00) >> 5);
	scale = mode->z == 8 ? 1 : (mode->z == 16? 2 : 4);
	if(pixbuswidth == 32)
		scale *= 2;
	scale--;
	mga->crtcext[3] = scale | 0x80;
	if(vga->vmz >= 8*Meg)
		mga->crtcext[3] |= 0x10;
	else if(vga->vmz == 2*Meg)
		mga->crtcext[3] |= 0x08;
	mga->crtcext[4] = 0;
	mga->crtcext[5] = 0;

	mga->tvp[0x0F] = (mode->z == 8) ? 0x06 : 0x07;
	mga->tvp[0x18] = (mode->z == 8) ? 0x80 :
		(mode->z == 16) ? 0x05 : 0x06;
	mga->tvp[0x19] = (mode->z == 8) ? 0x4C :
		(mode->z == 16) ? 0x54 : 0x5C;
	if(pixbuswidth == 32)
		mga->tvp[0x19]--;
	mga->tvp[0x1A] = (mode->z == 8) ? 0x25 :
		(mode->z == 16) ? 0x15 : 0x05;
	mga->tvp[0x1C] = 0;
	mga->tvp[0x1D] = (mode->y < 768) ? 0x00 : 0x03;
	mga->tvp[0x1E] = (mode->z == 8) ? 0x04 : 0x24; /* six bit mode */
	mga->tvp[0x2A] = 0;
	mga->tvp[0x2B] = 0x1E;
	mga->tvp[0x30] = 0xFF;
	mga->tvp[0x31] = 0xFF;
	mga->tvp[0x32] = 0xFF;
	mga->tvp[0x33] = 0xFF;
	mga->tvp[0x34] = 0xFF;
	mga->tvp[0x35] = 0xFF;
	mga->tvp[0x36] = 0xFF;
	mga->tvp[0x37] = 0xFF;
	mga->tvp[0x38] = 0;
	mga->tvp[0x39] = 0;
	mga->tvp[0x3A] = 0;
	mga->tvp[0x06] = 0;

	// From: "Bruce G. Stewart" <bruce.g.stewart@worldnet.att.net>
	// 05-Jul-00 - Fix vertical blanking setup
	// This should probably be corrected in vga.c too.
	{
		// Start vertical blanking after the last displayed line
		// End vertical blanking after the total line count
		int svb = mode->y;
		int evb = mode->vt;

		// A field is 1/2 of the lines in interlaced mode
		if(mode->interlace == 'v'){
			svb /= 2;
			evb /= 2;
		}
		--svb;
		--evb;			// line counter counts from 0

		vga->crt[0x15] = svb;
		vga->crt[0x07] = (vga->crt[0x07] & ~0x08) | ((svb & 0x100)>>5);
		vga->crt[0x09] = (vga->crt[0x09] & ~0x20) | ((svb & 0x200)>>4);
		// MGA specific: bits 10 and 11
		mga->crtcext[0x02] &=  ~0x18;
		mga->crtcext[0x02] |= ((svb & 0xC00)>>7);

		vga->crt[0x16]     = evb;
	}

	clockcalc(vga, ctlr, mode->z);


	mga->option = mga->option & ~0x3000;
	if(vga->vmz > 2*Meg)
		mga->option |= 0x1000;

	/* disable vga load (want to do fields in different order) */
	for(c = vga->link; c; c = c->link)
		if(c->name == "vga")
			c->load = nil;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int i;
	Mga *mga;
	Pcidev* pci;

	mga = vga->private;
	pci = mga->pci;

	trace("loading crtcext0-5\n");
	/* turn on mga mode, set other crt high bits */
	for(i = 0; i < 6; i++)
		crtcexto(i, mga->crtcext[i]);

	trace("setting memory mode\n");
	/* set memory mode */
	pcicfgw32(pci, Option, mga->option);

	/* clocksource: pixel clock PLL */
	_tvp3026xo(vga, ctlr, 0x1A, mga->tvp[0x1A]);

	/* disable pclk and lclk */
	_tvp3026xo(vga, ctlr, 0x2C, 0x2A);
	_tvp3026xo(vga, ctlr, 0x2F, 0);
	_tvp3026xo(vga, ctlr, 0x2D, 0);

	trace("loading vga registers\n");
	/* vga misc, seq, and registers */
	vgao(MiscW, vga->misc);

	for(i = 2; i < 0x05; i++)
		vgaxo(Seqx, i, vga->sequencer[i]);

	vgaxo(Crtx, 0x11, vga->crt[0x11] & ~0x80);
	for(i = 0; i < 0x19; i++)
		vgaxo(Crtx, i, vga->crt[i]);

	/* apparently AGP needs this to restore start address */
	crtcexto(0, mga->crtcext[0]);

	trace("programming pclk\n");
	/* set up pclk */
	_tvp3026xo(vga, ctlr, 0x2C, 0);
	for(i = 0; i < 3; i++)
		_tvp3026xo(vga, ctlr, 0x2D, mga->pclk[i]);
	while(!(_tvp3026xi(vga, ctlr, 0x2D) & 0x40))
		;

	trace("programming lclk\n");
	/* set up lclk */
	_tvp3026xo(vga, ctlr, 0x39, mga->tvp[0x39]);
	_tvp3026xo(vga, ctlr, 0x2C, 0);
	for(i = 0; i < 3; i++)
		_tvp3026xo(vga, ctlr, 0x2F, mga->lclk[i]);
	while(!(_tvp3026xi(vga, ctlr, 0x2F) & 0x40))
		;

	trace("loading tvp registers\n");
	/* tvp registers, order matters */
	_tvp3026xo(vga, ctlr, 0x0F, mga->tvp[0x0F]);
	_tvp3026xo(vga, ctlr, 0x18, mga->tvp[0x18]);
	_tvp3026xo(vga, ctlr, 0x19, mga->tvp[0x19]);
	_tvp3026xo(vga, ctlr, 0x1A, mga->tvp[0x1A]);
	_tvp3026xo(vga, ctlr, 0x1C, mga->tvp[0x1C]);
	_tvp3026xo(vga, ctlr, 0x1D, mga->tvp[0x1D]);
	_tvp3026xo(vga, ctlr, 0x1E, mga->tvp[0x1E]);
	_tvp3026xo(vga, ctlr, 0x2A, mga->tvp[0x2A]);
	_tvp3026xo(vga, ctlr, 0x2B, mga->tvp[0x2B]);
	_tvp3026xo(vga, ctlr, 0x30, mga->tvp[0x30]);
	_tvp3026xo(vga, ctlr, 0x31, mga->tvp[0x31]);
	_tvp3026xo(vga, ctlr, 0x32, mga->tvp[0x32]);
	_tvp3026xo(vga, ctlr, 0x33, mga->tvp[0x33]);
	_tvp3026xo(vga, ctlr, 0x34, mga->tvp[0x34]);
	_tvp3026xo(vga, ctlr, 0x35, mga->tvp[0x35]);
	_tvp3026xo(vga, ctlr, 0x36, mga->tvp[0x36]);
	_tvp3026xo(vga, ctlr, 0x37, mga->tvp[0x37]);
	_tvp3026xo(vga, ctlr, 0x38, mga->tvp[0x38]);
	_tvp3026xo(vga, ctlr, 0x39, mga->tvp[0x39]);
	_tvp3026xo(vga, ctlr, 0x3A, mga->tvp[0x3A]);
	_tvp3026xo(vga, ctlr, 0x06, mga->tvp[0x06]);

	trace("done mga load\n");
	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;
	Mga *mga;

	name = ctlr->name;
	mga = vga->private;

	printitem(name, "Devctrl");
	printreg(mga->devctrl);

	printitem(name, "Option");
	printreg(mga->option);

	printitem(name, "Crtcext");
	for(i = 0; i < 0x06; i++)
		printreg(mga->crtcext[i]);

	printitem(name, "TVP");
	for(i = 0; i < 64; i++)
		printreg(mga->tvp[i]);

	printitem(name, "PCLK");
	for(i = 0; i < 3; i++)
		printreg(mga->pclk[i]);

	printitem(name, "MCLK");
	for(i = 0; i < 3; i++)
		printreg(mga->mclk[i]);

	printitem(name, "LCLK");
	for(i = 0; i < 3; i++)
		printreg(mga->lclk[i]);
}

Ctlr mga2164w = {
	"mga2164w",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr mga2164whwgc = {
	"mga2164whwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	dump,				/* dump */
};
