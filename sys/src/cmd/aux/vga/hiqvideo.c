#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Chips and Technologies HiQVideo.
 * Extensions in the 69000 over the 65550 are not fully taken into account.
 * Depths other than 8 are too slow.
 */
enum {
	Frx		= 0x3D0,	/* Flat Panel Extensions Index */
	Mrx		= 0x3D2,	/* Multimedia Extensions Index */
	Xrx		= 0x3D6,	/* Configuration Extensions Index */
};

typedef struct {
	Pcidev* pci;

	uchar	fr[256];
	uchar	mr[256];
	uchar	xr[256];
} HiQVideo;

static uchar
hiqvideoxi(long port, uchar index)
{
	uchar data;

	outportb(port, index);
	data = inportb(port+1);

	return data;
}

static void
hiqvideoxo(long port, uchar index, uchar data)
{
	outportb(port, index);
	outportb(port+1, data);
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Pcidev *p;
	HiQVideo *hqv;

	if(vga->private == nil){
		vga->private = alloc(sizeof(HiQVideo));
		hqv = vga->private;
		if((p = pcimatch(nil, 0x102C, 0)) == nil)
			error("%s: not found\n", ctlr->name);
		switch(p->did){
		case 0x00C0:		/* 69000 HiQVideo */
			vga->f[1] = 135000000;
			vga->vmz = 2048*1024;
			break;
		case 0x00E0:		/* 65550 HiQV32 */
		case 0x00E4:		/* 65554 HiQV32 */
		case 0x00E5:		/* 65555 HiQV32 */
			vga->f[1] = 90000000;
			vga->vmz = 2048*1024;
			break;
		default:
			error("%s: DID %4.4uX unsupported\n",
				ctlr->name, p->did);
		}
		hqv->pci = p;
	}
	hqv = vga->private;

	for(i = 0; i < 0x50; i++)
		hqv->fr[i] = hiqvideoxi(Frx, i);
	for(i = 0; i < 0x60; i++)
		hqv->mr[i] = hiqvideoxi(Mrx, i);
	for(i = 0x30; i < 0x80; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	for(i = 0; i < 0x100; i++)
		hqv->xr[i] = hiqvideoxi(Xrx, i);

	switch(hqv->pci->did){
	case 0x00C0:			/* 69000 HiQVideo */
		vga->f[1] = 135000000;
		vga->vmz = 2048*1024;
		break;
	case 0x00E0:			/* 65550 HiQV32 */
	case 0x00E4:			/* 65554 HiQV32 */
	case 0x00E5:			/* 65555 HiQV32 */
		/*
		 * Check VCC to determine max clock.
		 * 5V allows a higher rate.
		 */
		if(hqv->fr[0x0A] & 0x02)
			vga->f[1] = 110000000;
		else
			vga->f[1] = 80000000;
		switch((hqv->xr[0x43]>>1) & 0x03){
		case 0:
			vga->vmz = 1*1024*1024;
			break;
		case 1:
			vga->vmz = 2*1024*1024;
			break;
		default:
			vga->vmz = 16*1024*1024;
			break;
		}
		break;
	}

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
	double f, fmin, fvco, m;
	ulong n, nmin, nmax, pd, rd;

	/*
	 * Constraints:
	 *  1.	1MHz <= Fref <= 60MHz
	 *  2.	150KHz <= Fref/(RD*N) <= 2MHz
	 *  3.	48MHz < Fvco <= 220MHz
	 *  4.	3 <= M <= 1023
	 *  5.	3 <= N <= 1023
	 */
	f = vga->f[0];

	/*
	 * Although pd = 0 is valid, things seems more stable
	 * starting at 1.
	 */
	for(pd = 1; pd < 6; pd++){
		f = vga->f[0]*(1<<pd);
		if(f > 48000000 && f <= 220000000)
			break;
	}
	if(pd >= 6)
		error("%s: dclk %lud out of range\n", ctlr->name, vga->f[0]);
	vga->p[0] = pd;

	/*
	 * The reference divisor has only two possible values, 1 and 4.
	 */
	fmin = f;
	for(rd = 1; rd <= 4; rd += 3){
		/*
		 * Find the range for n (constraint 2).
		 */
		for(nmin = 3; nmin <= 1023; nmin++){
			if(RefFreq/(rd*nmin) <= 2000000)
				break;
		}
		for(nmax = 1023; nmax >= 3; nmax--){
			if(RefFreq/(rd*nmax) >= 150000)
				break;
		}

		/*
		 * Now find the closest match for M and N.
		 */
		for(n = nmin; n < nmax; n++){
			for(m = 3; m <= vga->m[1]; m++){
				fvco = (RefFreq*4*m)/(rd*n);
				if(fvco < 48000000 || fvco > 220000000)
					continue;
				fvco -= f;
				if(fvco < 0)
					fvco = -fvco;
				if(fvco < fmin){
					fmin = fvco;
					vga->m[0] = m;
					vga->n[0] = n;
					vga->p[0] = pd;
					vga->r[0] = rd;
				}
			}
		}
	}

}

static void
init(Vga* vga, Ctlr* ctlr)
{
	HiQVideo *hqv;

	hqv = vga->private;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	vga->misc &= ~0x0C;
	hqv->fr[0x03] &= ~0x0C;

	switch(vga->mode->z){
	case 8:
	case 16:
	case 32:
		break;
	default:
		error("depth %d not supported\n", vga->mode->z);
	}

	/*
	 * FR[0x01] == 1 for CRT, 2 for LCD.
	 * Don't programme the clock if it's an LCD.
	 */
	if((hqv->fr[0x01] & 0x03) == 0x02){
		vga->misc |= 0x08;
		hqv->fr[0x03] |= 0x08;
	}
	else{
		/*
		 * Clock bits. If the desired video clock is
		 * one of the two standard VGA clocks it can just be
		 * set using bits <3:2> of vga->misc or Fr03,
		 * otherwise the DCLK PLL needs to be programmed.
		 */
		if(vga->f[0] == VgaFreq0){
			/* nothing to do */;
		}
		else if(vga->f[0] == VgaFreq1){
			vga->misc |= 0x04;
			hqv->fr[0x03] |= 0x04;
		}
		else{
			if(vga->f[0] > vga->f[1])
				error("%s: invalid dclk - %lud\n",
					ctlr->name, vga->f[0]);
	
			if((hqv->fr[0x01] & 0x03) != 0x02){
				vga->m[1] = 1023;
				clock(vga, ctlr);
				hqv->xr[0xC8] = (vga->m[0]-2) & 0xFF;
				hqv->xr[0xCA] = ((vga->m[0]-2) & 0x300)>>8;
				hqv->xr[0xC9] = (vga->n[0]-2) & 0xFF;
				hqv->xr[0xCA] |= ((vga->n[0]-2) & 0x300)>>4;
				hqv->xr[0xCB] = (vga->p[0]<<4)|(vga->r[0] == 1);
			}
			vga->misc |= 0x08;
			hqv->fr[0x03] |= 0x08;
		}
	}

	if(vga->mode->y >= 480)
		vga->misc &= ~0xC0;

	/*
	 * 10 bits should be enough, but set the extended mode anyway.
	 */
	hqv->xr[0x09] |= 0x01;
	vga->crt[0x30] = ((vga->mode->vt-2)>>8) & 0x0F;
	vga->crt[0x31] = ((vga->mode->y-1)>>8) & 0x0F;
	vga->crt[0x32] = (vga->mode->vrs>>8) & 0x0F;
	vga->crt[0x33] = (vga->mode->vrs>>8) & 0x0F;
	vga->crt[0x38] = (((vga->mode->ht>>3)-5)>>8) & 0x01;
	vga->crt[0x3C] = (vga->mode->ehb>>3) & 0xC0;
	vga->crt[0x41] = (vga->crt[0x13]>>8) & 0x0F;
	vga->crt[0x40] = 0x80;

	hqv->xr[0x40] |= 0x03;
	hqv->xr[0x80] |= 0x10;
	hqv->xr[0x81] &= ~0x0F;
	switch(vga->mode->z){
	case 8:
		hqv->xr[0x81] |= 0x12;
		break;
	case 16:
		hqv->xr[0x81] |= 0x15;
		break;
	case 32:
		hqv->xr[0x81] |= 0x16;
		break;
	}

	hqv->xr[0x0A] = 0x01;
	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	vga->attribute[0x11] = 0;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	HiQVideo *hqv;

	hqv = vga->private;

	hiqvideoxo(Xrx, 0x0E, 0x00);
	while((vgai(Status1) & 0x08) == 0x08)
		;
	while((vgai(Status1) & 0x08) == 0)
		;
	vgaxo(Seqx, 0x07, 0x00);

	/*
	 * Set the clock if necessary.
	 */
	if((vga->misc & 0x0C) == 0x08 && (hqv->fr[0x01] & 0x03) != 0x02){
		vgao(MiscW, vga->misc & ~0x0C);
		hiqvideoxo(Frx, 0x03, hqv->fr[0x03] & ~0x0C);

		hiqvideoxo(Xrx, 0xC8, hqv->xr[0xC8]);
		hiqvideoxo(Xrx, 0xC9, hqv->xr[0xC9]);
		hiqvideoxo(Xrx, 0xCA, hqv->xr[0xCA]);
		hiqvideoxo(Xrx, 0xCB, hqv->xr[0xCB]);
	}
	hiqvideoxo(Frx, 0x03, hqv->fr[0x03]);
	vgao(MiscW, vga->misc);

	hiqvideoxo(Xrx, 0x09, hqv->xr[0x09]);
	vgaxo(Crtx, 0x30, vga->crt[0x30]);
	vgaxo(Crtx, 0x31, vga->crt[0x31]);
	vgaxo(Crtx, 0x32, vga->crt[0x32]);
	vgaxo(Crtx, 0x33, vga->crt[0x33]);
	vgaxo(Crtx, 0x38, vga->crt[0x38]);
	vgaxo(Crtx, 0x3C, vga->crt[0x3C]);
	vgaxo(Crtx, 0x41, vga->crt[0x41]);
	vgaxo(Crtx, 0x40, vga->crt[0x40]);

	hiqvideoxo(Xrx, 0x40, hqv->xr[0x40]);
	hiqvideoxo(Xrx, 0x80, hqv->xr[0x80]);
	hiqvideoxo(Xrx, 0x81, hqv->xr[0x81]);
	if(ctlr->flag & Ulinear){
		hqv->xr[0x05] = vga->vmb>>16;
		hqv->xr[0x06] = vga->vmb>>24;
		hiqvideoxo(Xrx, 0x05, hqv->xr[0x05]);
		hiqvideoxo(Xrx, 0x06, hqv->xr[0x06]);
		hqv->xr[0x0A] = 0x02;
	}
	hiqvideoxo(Xrx, 0x0A, hqv->xr[0x0A]);

	ctlr->flag |= Fload;
}

static ulong
dumpmclk(uchar data[4])
{
	double f, m, n;
	int pds, rds;

	m = data[0] & 0x7F;
	n = data[1] & 0x7F;
	pds = 1<<((data[2] & 0x70)>>4);
	if(data[2] & 0x01)
		rds = 1;
	else
		rds = 4;
	f = (RefFreq*4*(m+2))/(rds*(n+2));
	f /= pds;

	return f;
}

static ulong
dumpvclk(uchar data[4])
{
	double f, m, n;
	int pds, rds;

	m = ((data[2] & 0x03)<<8)|data[0];
	n = (((data[2] & 0x30)>>4)<<8)|data[1];
	pds = 1<<((data[3] & 0x70)>>4);
	if(data[3] & 0x01)
		rds = 1;
	else
		rds = 4;
	f = (RefFreq*4*(m+2))/(rds*(n+2));
	f /= pds;

	return f;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char *name;
	HiQVideo *hqv;

	name = ctlr->name;
	hqv = vga->private;

	printitem(name, "Fr00");
	for(i = 0; i < 0x50; i++)
		printreg(hqv->fr[i]);
	printitem(name, "Mr00");
	for(i = 0; i < 0x60; i++)
		printreg(hqv->mr[i]);
	printitem(name, "Crt30");
	for(i = 0x30; i < 0x80; i++)
		printreg(vga->crt[i]);
	printitem(name, "Xr00");
	for(i = 0; i < 0x100; i++)
		printreg(hqv->xr[i]);

	printitem(ctlr->name, "CLK0");
	for(i = 0; i < 4; i++)
		printreg(hqv->xr[0xC0+i]);
	Bprint(&stdout, "%23ld", dumpvclk(&hqv->xr[0xC0]));
	printitem(ctlr->name, "CLK1");
	for(i = 0; i < 4; i++)
		printreg(hqv->xr[0xC4+i]);
	Bprint(&stdout, "%23ld", dumpvclk(&hqv->xr[0xC4]));
	printitem(ctlr->name, "CLK2");
	for(i = 0; i < 4; i++)
		printreg(hqv->xr[0xC8+i]);
	Bprint(&stdout, "%23ld", dumpvclk(&hqv->xr[0xC8]));
	printitem(ctlr->name, "MCLK");
	for(i = 0; i < 4; i++)
		printreg(hqv->xr[0xCC+i]);
	Bprint(&stdout, "%23ld", dumpmclk(&hqv->xr[0xCC]));

	printitem(ctlr->name, "m n pd rd");
	Bprint(&stdout, "%9ld %8ld       - %8ld %8ld\n",
		vga->m[0], vga->n[0], vga->p[0], vga->r[0]);
}

Ctlr hiqvideo = {
	"hiqvideo",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr hiqvideohwgc = {
	"hiqvideohwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
