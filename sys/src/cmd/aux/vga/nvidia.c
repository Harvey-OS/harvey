/* Portions of this file derived from work with the following copyright */

 /***************************************************************************\
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/

#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

typedef struct Nvidia	Nvidia;
struct Nvidia {
	Pcidev*	pci;
	int	did;	/* not always == pci->did */

	int	arch;
	int	crystalfreq;

	ulong*	mmio;
	ulong*	pfb;			/* mmio pointers */
	ulong*	pramdac;
	ulong*	pextdev;
	ulong*	pmc;
	ulong*	ptimer;
	ulong*	pfifo;
	ulong*	pramin;
	ulong*	pgraph;
	ulong*	fifo;
	ulong*	pcrtc;

	ushort	repaint0;
	ushort	repaint1;
	ushort	screen;
	ushort	pixel;
	ushort	horiz;
	ushort	cursor0;
	ushort	cursor1;
	ushort	cursor2;
	ushort	interlace;
	ushort	extra;
	ushort	crtcowner;
	ushort	timingH;
	ushort	timingV;

	ulong	vpll;
	ulong	vpllB;
	ulong	vpll2;
	ulong	vpll2B;
	ulong	pllsel;
	ulong	general;
	ulong	scale;
	ulong	config;
	ulong	head;
	ulong	head2;
	ulong	cursorconfig;
	ulong	dither;
	ulong	crtcsync;
	ulong	displayV;

	int	islcd;
	int	fpwidth;
	int	fpheight;
	int	twoheads;
	int	twostagepll;
	int	crtcnumber;
};

static void
getpcixdid(Nvidia* nv)
{
	ulong	pcicmd, pciid;
	ushort	vid, did;

	pcicmd = pcicfgr32(nv->pci, PciPCR);
	pcicfgw32(nv->pci, PciPCR, pcicmd | 0x02);
	pciid = nv->mmio[0x1800/4];
	pcicfgw32(nv->pci, PciPCR, pcicmd);

	vid = pciid >> 16;
	did = (pciid & 0xFFFF);
	if (did == 0x10DE)
		did = vid;
	else if (vid == 0xDE10)
		did = ((pciid << 8) & 0xFF00) | ((pciid >> 8) & 0x00FF);

	nv->did = did;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	Pcidev *p;
	ulong *mmio, tmp;
	int implementation;

	if(vga->private == nil){
		vga->private = alloc(sizeof(Nvidia));
		nv = vga->private;

		p = nil;
		while((p = pcimatch(p, 0x10DE, 0)) != nil){
			if((p->ccru>>8) == 3)
				break;
		}
		if(p == nil)
			error("%s: not found\n", ctlr->name);

		vgactlw("type", ctlr->name);

		mmio = segattach(0, "nvidiammio", 0, p->mem[0].size);
		if(mmio == (void*)-1)
			error("%s: segattach nvidiammio, size %d: %r\n",
				ctlr->name, p->mem[0].size);

		nv->pci = p;
		nv->mmio = mmio;

		nv->pfb = mmio+0x00100000/4;
		nv->pramdac = mmio+0x00680000/4;
		nv->pextdev = mmio+0x00101000/4;
		nv->pmc = mmio;
		nv->ptimer = mmio+0x00009000/4;
		nv->pfifo = mmio+0x00002000/4;
		nv->pramin = mmio+0x00710000/4;
		nv->pgraph = mmio+0x00400000/4;
		nv->fifo = mmio+0x00800000/4;
		nv->pcrtc= mmio+0x00600000/4;

		nv->did = p->did;
		if ((nv->did & 0xfff0) == 0x00f0)
			getpcixdid(nv);

		switch (nv->did & 0x0ff0) {
		case 0x0020:
		case 0x00A0:
			nv->arch = 4;
			break;
		case 0x0100:	/* GeForce 256 */
		case 0x0110:	/* GeForce2 MX */
		case 0x0150:	/* GeForce2 */
		case 0x0170:	/* GeForce4 MX */
		case 0x0180:	/* GeForce4 MX (8x AGP) */
		case 0x01A0:	/* nForce */
		case 0x01F0:	/* nForce2 */
			nv->arch = 10;
			break;
		case 0x0200:	/* GeForce3 */
		case 0x0250:	/* GeForce4 Ti */
		case 0x0280:	/* GeForce4 Ti (8x AGP) */
			nv->arch = 20;
			break;
		case 0x0300:	/* GeForceFX 5800 */
		case 0x0310:	/* GeForceFX 5600 */
		case 0x0320:	/* GeForceFX 5200 */
		case 0x0330:	/* GeForceFX 5900 */
		case 0x0340:	/* GeForceFX 5700 */
			nv->arch = 30;
			break;
		case 0x0040:
		case 0x0090:
		case 0x00C0:
		case 0x0120:
		case 0x0130:
		case 0x0140:	/* GeForce 6600 */
		case 0x0160:
		case 0x01D0:
		case 0x0210:
		case 0x0290:	/* nvidia 7950 */
		case 0x0390:
			nv->arch = 40;
			break;
		default:
			error("%s: DID %#4.4ux - %#ux unsupported\n",
				ctlr->name, nv->did, (nv->did & 0x0ff0));
			break;
		}
	}
	nv = vga->private;
	implementation = nv->did & 0x0ff0;

	/*
	 * Unlock
	 */
	vgaxo(Crtx, 0x1F, 0x57);

	if (nv->pextdev[0] & 0x40)
		nv->crystalfreq = RefFreq;
	else
		nv->crystalfreq = 13500000;

	if ((implementation == 0x0170) ||
	    (implementation == 0x0180) ||
	    (implementation == 0x01F0) ||
	    (implementation >= 0x0250))
		if(nv->pextdev[0] & (1 << 22))
			nv->crystalfreq = 27000000;

	nv->twoheads = (nv->arch >= 10) &&
			(implementation != 0x0100) &&
			(implementation != 0x0150) &&
			(implementation != 0x01A0) &&
			(implementation != 0x0200);

	nv->twostagepll = (implementation == 0x0310) ||
			(implementation == 0x0340) || (nv->arch >= 40);

	if (nv->twoheads && (implementation != 0x0110))
		if(nv->pextdev[0] & (1 << 22))
			nv->crystalfreq = 27000000;

	/* laptop chips */
	switch (nv->did & 0xffff) {
	case 0x0112:
	case 0x0174:
	case 0x0175:
	case 0x0176:
	case 0x0177:
	case 0x0179:
	case 0x017C:
	case 0x017D:
	case 0x0186:
	case 0x0187:
	case 0x0189:	/* 0x0189 not in nwaples's driver */
	case 0x018D:
	case 0x0286:
	case 0x028C:
	case 0x0316:
	case 0x0317:
	case 0x031A:
	case 0x031B:
	case 0x031C:
	case 0x031D:
	case 0x031E:
	case 0x031F:
	case 0x0324:
	case 0x0325:
	case 0x0328:
	case 0x0329:
	case 0x032C:
	case 0x032D:
	case 0x0347:
	case 0x0348:
	case 0x0349:
	case 0x034B:
	case 0x034C:
	case 0x0160:
	case 0x0166:
	case 0x00C8:
	case 0x00CC:
	case 0x0144:
	case 0x0146:
	case 0x0148:
	case 0x01D7:
		nv->islcd = 1;
		break;
	default:
		break;
	}

	if (nv->arch == 4) {
		tmp = nv->pfb[0];
		if (tmp & 0x0100)
			vga->vmz = ((tmp >> 12) & 0x0F)*1024 + 2*1024;
		else {
			tmp &= 0x03;
			if (tmp)
				vga->vmz = (1024*1024*2) << tmp;
			else
				vga->vmz = 1024*1024*32;
		}
	} else if (implementation == 0x01a0) {
		p = nil;
		tmp = MKBUS(BusPCI, 0, 0, 1);
		while((p = pcimatch(p, 0x10DE, 0)) != nil){
			if(p->tbdf == tmp)
				break;
		}
		tmp = pcicfgr32(p, 0x7C);
		vga->vmz = (((tmp >> 6) & 31) + 1) * 1024 * 1024;
	} else if (implementation == 0x01f0) {
		p = nil;
		tmp = MKBUS(BusPCI, 0, 0, 1);
		while((p = pcimatch(p, 0x10DE, 0)) != nil){
			if(p->tbdf == tmp)
				break;
		}
		tmp = pcicfgr32(p, 0x84);
		vga->vmz = (((tmp >> 4) & 127) + 1) * 1024*1024;
	} else {
		tmp = (nv->pfb[0x0000020C/4] >> 20) & 0xFFF;
		if (tmp == 0)
			tmp = 16;
		vga->vmz = 1024*1024*tmp;
	}

	nv->repaint0 = vgaxi(Crtx, 0x19);
	nv->repaint1 = vgaxi(Crtx, 0x1A);
	nv->screen = vgaxi(Crtx, 0x25);
	nv->pixel = vgaxi(Crtx, 0x28);
	nv->horiz = vgaxi(Crtx, 0x2D);
	nv->cursor0 = vgaxi(Crtx, 0x30);
	nv->cursor1 = vgaxi(Crtx, 0x31);
	nv->cursor2 = vgaxi(Crtx, 0x2F);
	nv->interlace = vgaxi(Crtx, 0x39);

	nv->vpll = nv->pramdac[0x508/4];
	if (nv->twoheads)
		nv->vpll2 = nv->pramdac[0x520/4];
	if (nv->twostagepll) {
		nv->vpllB = nv->pramdac[0x578/4];
		nv->vpll2B = nv->pramdac[0x57C/4];
	}
	nv->pllsel = nv->pramdac[0x50C/4];
	nv->general = nv->pramdac[0x600/4];
	nv->scale = nv->pramdac[0x848/4];
	nv->config = nv->pfb[0x200/4];

	if (nv->pixel & 0x80)
		nv->islcd = 1;

	if (nv->arch >= 10) {
		if (nv->twoheads) {
			nv->head = nv->pcrtc[0x0860/4];
			nv->head2 = nv->pcrtc[0x2860/4];
			nv->crtcowner = vgaxi(Crtx, 0x44);
		}
		nv->extra = vgaxi(Crtx, 0x41);
		nv->cursorconfig = nv->pcrtc[0x0810/4];
		if (implementation == 0x0110)
			nv->dither = nv->pramdac[0x0528/4];
		else if (nv->twoheads)
			nv->dither = nv->pramdac[0x083C/4];
		if(nv->islcd){
			nv->timingH = vgaxi(Crtx, 0x53);
			nv->timingV = vgaxi(Crtx, 0x54);
		}
	}

	/*
	 * DFP.
	 */
	if (nv->islcd) {
		nv->fpwidth = nv->pramdac[0x0820/4] + 1;
		nv->fpheight = nv->pramdac[0x0800/4] + 1;
		nv->crtcsync = nv->pramdac[0x0828/4];
	}

	nv->crtcnumber = 0;

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
	int fmin, mmin, nmin, crystalfreq;
	nv = vga->private;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	vga->d[0] = vga->f[0]+1;

	vga->n[1] = 255;
	if (nv->twostagepll) {
		vga->p[1] = 6;
		vga->m[1] = 13;
		vga->f[1] = 400000000 << 2;
		crystalfreq = nv->crystalfreq << 2;
		fmin = 100000000 << 2;
		mmin = 1;
		nmin = 5;
		nv->vpllB = 0x80000401;
	} else {
		vga->p[1] = 4;
		if (nv->crystalfreq == 13500000)
			vga->m[1] = 13;
		else
			vga->m[1] = 14;
		vga->f[1] = 350000000;
		crystalfreq = nv->crystalfreq;
		fmin = 128000000;
		mmin = 7;
		nmin = 0;
	}

	for (p=0; p <= vga->p[1]; p++){
		f = vga->f[0] << p;
		if ((f >= fmin) && (f <= vga->f[1])) {
			for (m=mmin; m <= vga->m[1]; m++){
				trouble = (double) crystalfreq / (double) (m << p);
				n = (vga->f[0] / trouble)+0.5;
				f = n*trouble + 0.5;
				d = vga->f[0] - f;
				if (d < 0)
					d = -d;
				if ((n & ~0xFF) && (n >= nmin))
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
	char *p, *val;
	int tmp, pixeldepth;
	ulong cursorstart;

	mode = vga->mode;
	if(mode->z == 24)
		error("%s: 24-bit colour not supported, use 32-bit\n", ctlr->name);

	nv = vga->private;

	if(vga->linear && (ctlr->flag & Hlinear))
		ctlr->flag |= Ulinear;

	clock(vga, ctlr);

	if(val = dbattr(vga->mode->attr, "lcd")){
		 if((nv->islcd = strtol(val, &p, 0)) == 0 && p == val)
			error("%s: invalid 'lcd' attr\n", ctlr->name);
	}

	if(nv->arch == 4) {
		nv->cursor0 = 0;
		nv->cursor1 = 0xBC;
		nv->cursor2 = 0;
		nv->config = 0x00001114;
	} else if(nv->arch >= 10) {
		cursorstart = vga->vmz - 96 * 1024;
		nv->cursor0 = 0x80 | (cursorstart >> 17);
		nv->cursor1 = (cursorstart >> 11) << 2;
		nv->cursor2 = cursorstart >> 24;
		nv->config = nv->pfb[0x200/4];
	}

	nv->vpll = (vga->p[0] << 16) | (vga->n[0] << 8) | vga->m[0];
	nv->pllsel = 0x10000700;
	if (mode->z == 16)
		nv->general = 0x00001100;
	else
		nv->general = 0x00000100;
	if (0 && mode->z != 8)
		nv->general |= 0x00000030;

	if (mode->x < 1280)
		nv->repaint1 = 0x04;
	else
		nv->repaint1 = 0;

	vga->attribute[0x10] &= ~0x40;
	vga->attribute[0x11] = Pblack;
	vga->crt[0x14] = 0;

	if(1 && vga->f[0] != VgaFreq0 && vga->f[1] != VgaFreq1)
		vga->misc |= 0x08;

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
	tmp = vga->crt[0] + 4;
	vga->crt[0x03] = 0x80 | (tmp & 0x1F);
	if (tmp & 0x20)
		vga->crt[0x05] |= 0x80;
	else
		vga->crt[0x05] &= ~0x80;
	if (tmp & 0x40)
		nv->screen = 0x10;
	else
		nv->screen = 0;

	/* overflow bits */

	if (nv->islcd){
		tmp = vga->crt[0x06] - 3;
		vga->crt[0x10] = tmp;
		if(tmp & 0x100)
			vga->crt[0x07] |= 0x04;
		else
			vga->crt[0x07] &= ~0x04;
		if(tmp & 0x200)
			vga->crt[0x07] |= 0x80;
		else
			vga->crt[0x07] &= ~0x80;

		vga->crt[0x11] = 0x20 | ((vga->crt[0x06] - 2) & 0x0F);

		tmp = vga->crt[0x10];
		vga->crt[0x15] = tmp;
		if(tmp & 0x100)
			vga->crt[0x07] |= 0x08;
		else
			vga->crt[0x07] &= ~0x08;
		if(tmp & 0x200)
			vga->crt[0x09] |= 0x20;
		else
			vga->crt[0x09] &= ~0x20;

		vga->crt[0x04] = vga->crt[0] - 5;

		vga->crt[0x05] &= ~0x1F;
		vga->crt[0x05] |= (0x1F & (vga->crt[0] - 2));
	}

	nv->repaint0 = (vga->crt[0x13] & 0x0700) >> 3;

	pixeldepth = (mode->z +1)/8;
	if (pixeldepth > 3)
		nv->pixel = 3;
	else
		nv->pixel = pixeldepth;

	nv->scale &= 0xFFF000FF;
	if(nv->islcd){
		nv->pixel |= 0x80;
		nv->scale |= 0x100;
	}

	if (vga->crt[0x06] & 0x400)
		nv->screen |= 0x01;
	if (vga->crt[0x12] & 0x400)
		nv->screen |= 0x02;
	if (vga->crt[0x10] & 0x400)
		nv->screen |= 0x04;
	if (vga->crt[0x15] & 0x400)
		nv->screen |= 0x08;
	if (vga->crt[0x13] & 0x800)
		nv->screen |= 0x20;

	nv->horiz = 0;
	if (vga->crt[0] & 0x100)
		nv->horiz = 0x01;
	if(vga->crt[0x01] & 0x100)
		nv->horiz |= 0x02;
	if(vga->crt[0x02] & 0x100)
		nv->horiz |= 0x04;
	if(vga->crt[0x04] & 0x100)
		nv->horiz |= 0x08;

	nv->extra = 0;
	if (vga->crt[0x06] & 0x800)
		nv->extra |= 0x01;
	if (vga->crt[0x12] & 0x800)
		nv->extra |= 0x04;
	if (vga->crt[0x10] & 0x800)
		nv->extra |= 0x10;
	if (vga->crt[0x15] & 0x800)
		nv->extra |= 0x40;

	nv->interlace = 0xFF;
	if (nv->twoheads) {
		nv->head |= 0x00001000;
		nv->head2 &= ~0x00001000;
		nv->crtcowner = 0;
		if((nv->did & 0x0ff0) == 0x0110)
			nv->dither &= ~0x00010000;
		else
			nv->dither &= ~1;
	}
	nv->cursorconfig = 0x00000100 | 0x02000000;

	nv->timingH = 0;
	nv->timingV = 0;
	nv->displayV = vga->crt[0x12] + 1;

	ctlr->flag |= Finit;
}


static void
load(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	int i, regions;
	ulong tmp;

	nv = vga->private;

	/*
	 * Unlock
	 */
	vgaxo(Crtx, 0x1F, 0x57);

 	nv->pmc[0x0140/4] = 0;
 	nv->pmc[0x0200/4] = 0xFFFF00FF;
 	nv->pmc[0x0200/4] = 0xFFFFFFFF;

 	nv->ptimer[0x0200] = 8;
 	nv->ptimer[0x0210] = 3;
 	nv->ptimer[0x0140] = 0;
 	nv->ptimer[0x0100] = 0xFFFFFFFF;

	if (nv->arch == 4)
		nv->pfb[0x00000200/4] = nv->config;
	else if((nv->arch < 40) || ((nv->did & 0xfff0) == 0x0040)){
		for(i = 0; i < 8; i++){
			nv->pfb[(0x0240 + (i * 0x10))/4] = 0;
			nv->pfb[(0x0244 + (i * 0x10))/4] = vga->vmz - 1;;
		}
	}
	else{
		if(((nv->did & 0xfff0) == 0x0090)
		|| ((nv->did & 0xfff0) == 0x01D0)
		|| ((nv->did & 0xfff0) == 0x0290)
		|| ((nv->did & 0xfff0) == 0x0390))
			regions = 15;
		else
			regions = 12;

		for(i = 0; i < regions; i++){
			nv->pfb[(0x0600 + (i * 0x10))/4] = 0;
			nv->pfb[(0x0604 + (i * 0x10))/4] = vga->vmz - 1;
		}
	}

	if (nv->arch >= 40) {
		nv->pramin[0] = 0x80000010;
		nv->pramin[0x0001] = 0x00101202;
		nv->pramin[0x0002] = 0x80000011;
		nv->pramin[0x0003] = 0x00101204;
		nv->pramin[0x0004] = 0x80000012;
		nv->pramin[0x0005] = 0x00101206;
		nv->pramin[0x0006] = 0x80000013;
		nv->pramin[0x0007] = 0x00101208;
		nv->pramin[0x0008] = 0x80000014;
		nv->pramin[0x0009] = 0x0010120A;
		nv->pramin[0x000A] = 0x80000015;
		nv->pramin[0x000B] = 0x0010120C;
		nv->pramin[0x000C] = 0x80000016;
		nv->pramin[0x000D] = 0x0010120E;
		nv->pramin[0x000E] = 0x80000017;
		nv->pramin[0x000F] = 0x00101210;
		nv->pramin[0x0800] = 0x00003000;
		nv->pramin[0x0801] = vga->vmz - 1;
		nv->pramin[0x0802] = 0x00000002;
		nv->pramin[0x0808] = 0x02080062;
		nv->pramin[0x0809] = 0;
		nv->pramin[0x080A] = 0x00001200;
		nv->pramin[0x080B] = 0x00001200;
		nv->pramin[0x080C] = 0;
		nv->pramin[0x080D] = 0;
		nv->pramin[0x0810] = 0x02080043;
		nv->pramin[0x0811] = 0;
		nv->pramin[0x0812] = 0;
		nv->pramin[0x0813] = 0;
		nv->pramin[0x0814] = 0;
		nv->pramin[0x0815] = 0;
		nv->pramin[0x0818] = 0x02080044;
		nv->pramin[0x0819] = 0x02000000;
		nv->pramin[0x081A] = 0;
		nv->pramin[0x081B] = 0;
		nv->pramin[0x081C] = 0;
		nv->pramin[0x081D] = 0;
		nv->pramin[0x0820] = 0x02080019;
		nv->pramin[0x0821] = 0;
		nv->pramin[0x0822] = 0;
		nv->pramin[0x0823] = 0;
		nv->pramin[0x0824] = 0;
		nv->pramin[0x0825] = 0;
		nv->pramin[0x0828] = 0x020A005C;
		nv->pramin[0x0829] = 0;
		nv->pramin[0x082A] = 0;
		nv->pramin[0x082B] = 0;
		nv->pramin[0x082C] = 0;
		nv->pramin[0x082D] = 0;
		nv->pramin[0x0830] = 0x0208009F;
		nv->pramin[0x0831] = 0;
		nv->pramin[0x0832] = 0x00001200;
		nv->pramin[0x0833] = 0x00001200;
		nv->pramin[0x0834] = 0;
		nv->pramin[0x0835] = 0;
		nv->pramin[0x0838] = 0x0208004A;
		nv->pramin[0x0839] = 0x02000000;
		nv->pramin[0x083A] = 0;
		nv->pramin[0x083B] = 0;
		nv->pramin[0x083C] = 0;
		nv->pramin[0x083D] = 0;
		nv->pramin[0x0840] = 0x02080077;
		nv->pramin[0x0841] = 0;
		nv->pramin[0x0842] = 0x00001200;
		nv->pramin[0x0843] = 0x00001200;
		nv->pramin[0x0844] = 0;
		nv->pramin[0x0845] = 0;
		nv->pramin[0x084C] = 0x00003002;
		nv->pramin[0x084D] = 0x00007FFF;
		nv->pramin[0x084E] = (vga->vmz - 128*1024) | 2;
	} else {
		nv->pramin[0x0000] = 0x80000010;
		nv->pramin[0x0001] = 0x80011201;
		nv->pramin[0x0002] = 0x80000011;
		nv->pramin[0x0003] = 0x80011202;
		nv->pramin[0x0004] = 0x80000012;
		nv->pramin[0x0005] = 0x80011203;
		nv->pramin[0x0006] = 0x80000013;
		nv->pramin[0x0007] = 0x80011204;
		nv->pramin[0x0008] = 0x80000014;
		nv->pramin[0x0009] = 0x80011205;
		nv->pramin[0x000A] = 0x80000015;
		nv->pramin[0x000B] = 0x80011206;
		nv->pramin[0x000C] = 0x80000016;
		nv->pramin[0x000D] = 0x80011207;
		nv->pramin[0x000E] = 0x80000017;
		nv->pramin[0x000F] = 0x80011208;
		nv->pramin[0x0800] = 0x00003000;
		nv->pramin[0x0801] = vga->vmz - 1;
		nv->pramin[0x0802] = 0x00000002;
		nv->pramin[0x0803] = 0x00000002;
		if (nv->arch >= 10)
			nv->pramin[0x0804] = 0x01008062;
		else
			nv->pramin[0x0804] = 0x01008042;
		nv->pramin[0x0805] = 0;
		nv->pramin[0x0806] = 0x12001200;
		nv->pramin[0x0807] = 0;
		nv->pramin[0x0808] = 0x01008043;
		nv->pramin[0x0809] = 0;
		nv->pramin[0x080A] = 0;
		nv->pramin[0x080B] = 0;
		nv->pramin[0x080C] = 0x01008044;
		nv->pramin[0x080D] = 0x00000002;
		nv->pramin[0x080E] = 0;
		nv->pramin[0x080F] = 0;
		nv->pramin[0x0810] = 0x01008019;
		nv->pramin[0x0811] = 0;
		nv->pramin[0x0812] = 0;
		nv->pramin[0x0813] = 0;
		nv->pramin[0x0814] = 0x0100A05C;
		nv->pramin[0x0815] = 0;
		nv->pramin[0x0816] = 0;
		nv->pramin[0x0817] = 0;
		nv->pramin[0x0818] = 0x0100805F;
		nv->pramin[0x0819] = 0;
		nv->pramin[0x081A] = 0x12001200;
		nv->pramin[0x081B] = 0;
		nv->pramin[0x081C] = 0x0100804A;
		nv->pramin[0x081D] = 0x00000002;
		nv->pramin[0x081E] = 0;
		nv->pramin[0x081F] = 0;
		nv->pramin[0x0820] = 0x01018077;
		nv->pramin[0x0821] = 0;
		nv->pramin[0x0822] = 0x01201200;
		nv->pramin[0x0823] = 0;
		nv->pramin[0x0824] = 0x00003002;
		nv->pramin[0x0825] = 0x00007FFF;
		nv->pramin[0x0826] = (vga->vmz - 128*1024) | 2;
		nv->pramin[0x0827] = 0x00000002;
	}
	if (nv->arch < 10) {
		if((nv->did & 0x0fff) == 0x0020) {
			nv->pramin[0x0824] |= 0x00020000;
			nv->pramin[0x0826] += nv->pci->mem[1].bar;
		}
		nv->pgraph[0x0080/4] = 0x000001FF;
		nv->pgraph[0x0080/4] = 0x1230C000;
		nv->pgraph[0x0084/4] = 0x72111101;
		nv->pgraph[0x0088/4] = 0x11D5F071;
		nv->pgraph[0x008C/4] = 0x0004FF31;
		nv->pgraph[0x008C/4] = 0x4004FF31;

		nv->pgraph[0x0140/4] = 0;
		nv->pgraph[0x0100/4] = 0xFFFFFFFF;
		nv->pgraph[0x0170/4] = 0x10010100;
		nv->pgraph[0x0710/4] = 0xFFFFFFFF;
		nv->pgraph[0x0720/4] = 1;

		nv->pgraph[0x0810/4] = 0;
		nv->pgraph[0x0608/4] = 0xFFFFFFFF;
	} else {
		nv->pgraph[0x0080/4] = 0xFFFFFFFF;
		nv->pgraph[0x0080/4] = 0;

		nv->pgraph[0x0140/4] = 0;
		nv->pgraph[0x0100/4] = 0xFFFFFFFF;
		nv->pgraph[0x0144/4] = 0x10010100;
		nv->pgraph[0x0714/4] = 0xFFFFFFFF;
		nv->pgraph[0x0720/4] = 1;
		nv->pgraph[0x0710/4] &= 0x0007ff00;
		nv->pgraph[0x0710/4] |= 0x00020100;

		if (nv->arch == 10) {
			nv->pgraph[0x0084/4] = 0x00118700;
			nv->pgraph[0x0088/4] = 0x24E00810;
			nv->pgraph[0x008C/4] = 0x55DE0030;

			for(i = 0; i < 32; i++)
				nv->pgraph[0x0B00/4 + i] = nv->pfb[0x0240/4 + i];

			nv->pgraph[0x640/4] = 0;
			nv->pgraph[0x644/4] = 0;
			nv->pgraph[0x684/4] = vga->vmz - 1;
			nv->pgraph[0x688/4] = vga->vmz - 1;

			nv->pgraph[0x0810/4] = 0;
			nv->pgraph[0x0608/4] = 0xFFFFFFFF;
		} else {
			if (nv->arch >= 40) {
				nv->pgraph[0x0084/4] = 0x401287c0;
				nv->pgraph[0x008C/4] = 0x60de8051;
				nv->pgraph[0x0090/4] = 0x00008000;
				nv->pgraph[0x0610/4] = 0x00be3c5f;


				tmp = nv->pmc[0x1540/4] & 0xff;
				for(i = 0; tmp && !(tmp & 1); tmp >>= 1, i++)
					;
				nv->pgraph[0x5000/4] = i;

				if ((nv->did & 0xfff0) == 0x0040) {
					nv->pgraph[0x09b0/4] = 0x83280fff;
					nv->pgraph[0x09b4/4] = 0x000000a0;
				} else {
					nv->pgraph[0x0820/4] = 0x83280eff;
					nv->pgraph[0x0824/4] = 0x000000a0;
				}

				switch(nv->did & 0xfff0) {
				case 0x0040:
					nv->pgraph[0x09b8/4] = 0x0078e366;
					nv->pgraph[0x09bc/4] = 0x0000014c;
					nv->pfb[0x033C/4] &= 0xffff7fff;
					break;
				case 0x00C0:
				case 0x0120:
					nv->pgraph[0x0828/4] = 0x007596ff;
					nv->pgraph[0x082C/4] = 0x00000108;
					break;
				case 0x0160:
				case 0x01D0:
				case 0x0240:
					nv->pmc[0x1700/4] = nv->pfb[0x020C/4];
					nv->pmc[0x1704/4] = 0;
					nv->pmc[0x1708/4] = 0;
					nv->pmc[0x170C/4] = nv->pfb[0x020C/4];
					nv->pgraph[0x0860/4] = 0;
					nv->pgraph[0x0864/4] = 0;
					nv->pramdac[0x0608/4] |= 0x00100000;
					break;
				case 0x0140:
					nv->pgraph[0x0828/4] = 0x0072cb77;
					nv->pgraph[0x082C/4] = 0x00000108;
					break;
				case 0x0220:
					nv->pgraph[0x0860/4] = 0;
					nv->pgraph[0x0864/4] = 0;
					nv->pramdac[0x0608/4] |= 0x00100000;
					break;
				case 0x0090:
				case 0x0290:
				case 0x0390:
					nv->pgraph[0x0608/4] |= 0x00100000;
					nv->pgraph[0x0828/4] = 0x07830610;
					nv->pgraph[0x082C/4] = 0x0000016A;
					break;
				default:
					break;
				}

				nv->pgraph[0x0b38/4] = 0x2ffff800;
				nv->pgraph[0x0b3c/4] = 0x00006000;
				nv->pgraph[0x032C/4] = 0x01000000;
				nv->pgraph[0x0220/4] = 0x00001200;
			} else if (nv->arch == 30) {
				nv->pgraph[0x0084/4] = 0x40108700;
				nv->pgraph[0x0890/4] = 0x00140000;
				nv->pgraph[0x008C/4] = 0xf00e0431;
				nv->pgraph[0x0090/4] = 0x00008000;
				nv->pgraph[0x0610/4] = 0xf04b1f36;
				nv->pgraph[0x0B80/4] = 0x1002d888;
				nv->pgraph[0x0B88/4] = 0x62ff007f;
			} else {
				nv->pgraph[0x0084/4] = 0x00118700;
				nv->pgraph[0x008C/4] = 0xF20E0431;
				nv->pgraph[0x0090/4] = 0;
				nv->pgraph[0x009C/4] = 0x00000040;

				if((nv->did & 0x0ff0) >= 0x0250) {
					nv->pgraph[0x0890/4] = 0x00080000;
					nv->pgraph[0x0610/4] = 0x304B1FB6;
					nv->pgraph[0x0B80/4] = 0x18B82880;
					nv->pgraph[0x0B84/4] = 0x44000000;
					nv->pgraph[0x0098/4] = 0x40000080;
					nv->pgraph[0x0B88/4] = 0x000000ff;
				} else {
					nv->pgraph[0x0880/4] = 0x00080000;
					nv->pgraph[0x0094/4] = 0x00000005;
					nv->pgraph[0x0B80/4] = 0x45CAA208;
					nv->pgraph[0x0B84/4] = 0x24000000;
					nv->pgraph[0x0098/4] = 0x00000040;
					nv->pgraph[0x0750/4] = 0x00E00038;
					nv->pgraph[0x0754/4] = 0x00000030;
					nv->pgraph[0x0750/4] = 0x00E10038;
					nv->pgraph[0x0754/4] = 0x00000030;
				}
			}

			if((nv->arch < 40) || ((nv->did & 0xfff0) == 0x0040)){
				for(i = 0; i < 32; i++) {
					nv->pgraph[(0x0900/4) + i] = nv->pfb[(0x0240/4) + i];
					nv->pgraph[(0x6900/4) + i] = nv->pfb[(0x0240/4) + i];
				}
			}
			else{
				if(((nv->did & 0xfff0) == 0x0090)
				|| ((nv->did & 0xfff0) == 0x01D0)
				|| ((nv->did & 0xfff0) == 0x0290)
				|| ((nv->did & 0xfff0) == 0x0390)){
					for(i = 0; i < 60; i++) {
						nv->pgraph[(0x0D00/4) + i] = nv->pfb[(0x0600/4) + i];
						nv->pgraph[(0x6900/4) + i] = nv->pfb[(0x0600/4) + i];
					}
				}
				else{
					for(i = 0; i < 48; i++) {
						nv->pgraph[(0x0900/4) + i] = nv->pfb[(0x0600/4) + i];
						if(((nv->did & 0xfff0) != 0x0160)
						&& ((nv->did & 0xfff0) != 0x0220)
						&& ((nv->did & 0xfff0) != 0x0240))
							nv->pgraph[(0x6900/4) + i] = nv->pfb[(0x0600/4) + i];
					}
				}
			}

			if(nv->arch >= 40) {
				if((nv->did & 0xfff0) == 0x0040) {
					nv->pgraph[0x09A4/4] = nv->pfb[0x0200/4];
					nv->pgraph[0x09A8/4] = nv->pfb[0x0204/4];
					nv->pgraph[0x69A4/4] = nv->pfb[0x0200/4];
					nv->pgraph[0x69A8/4] = nv->pfb[0x0204/4];

					nv->pgraph[0x0820/4] = 0;
					nv->pgraph[0x0824/4] = 0;
					nv->pgraph[0x0864/4] = vga->vmz - 1;
					nv->pgraph[0x0868/4] = vga->vmz - 1;
				} else {
					nv->pgraph[0x09F0/4] = nv->pfb[0x0200/4];
					nv->pgraph[0x09F4/4] = nv->pfb[0x0204/4];
					nv->pgraph[0x69F0/4] = nv->pfb[0x0200/4];
					nv->pgraph[0x69F4/4] = nv->pfb[0x0204/4];

					nv->pgraph[0x0840/4] = 0;
					nv->pgraph[0x0844/4] = 0;
					nv->pgraph[0x08a0/4] = vga->vmz - 1;
					nv->pgraph[0x08a4/4] = vga->vmz - 1;
				}
			} else {
				nv->pgraph[0x09A4/4] = nv->pfb[0x0200/4];
				nv->pgraph[0x09A8/4] = nv->pfb[0x0204/4];
				nv->pgraph[0x0750/4] = 0x00EA0000;
				nv->pgraph[0x0754/4] = nv->pfb[0x0200/4];
				nv->pgraph[0x0750/4] = 0x00EA0004;
				nv->pgraph[0x0754/4] = nv->pfb[0x0204/4];

				nv->pgraph[0x0820/4] = 0;
				nv->pgraph[0x0824/4] = 0;
				nv->pgraph[0x0864/4] = vga->vmz - 1;
				nv->pgraph[0x0868/4] = vga->vmz - 1;
			}

			nv->pgraph[0x0B20/4] = 0;
			nv->pgraph[0x0B04/4] = 0xFFFFFFFF;
		}
	}

	nv->pgraph[0x053C/4] = 0;
	nv->pgraph[0x0540/4] = 0;
	nv->pgraph[0x0544/4] = 0x00007FFF;
	nv->pgraph[0x0548/4] = 0x00007FFF;

	nv->pfifo[0x0140] = 0;
	nv->pfifo[0x0141] = 0x00000001;
	nv->pfifo[0x0480] = 0;
	nv->pfifo[0x0494] = 0;
	if (nv->arch >= 40)
		nv->pfifo[0x0481] = 0x00010000;
	else
		nv->pfifo[0x0481] = 0x00000100;
	nv->pfifo[0x0490] = 0;
	nv->pfifo[0x0491] = 0;
	if (nv->arch >= 40)
		nv->pfifo[0x048B] = 0x00001213;
	else
		nv->pfifo[0x048B] = 0x00001209;
	nv->pfifo[0x0400] = 0;
	nv->pfifo[0x0414] = 0;
	nv->pfifo[0x0084] = 0x03000100;
	nv->pfifo[0x0085] = 0x00000110;
	nv->pfifo[0x0086] = 0x00000112;
	nv->pfifo[0x0143] = 0x0000FFFF;
	nv->pfifo[0x0496] = 0x0000FFFF;
	nv->pfifo[0x0050] = 0;
	nv->pfifo[0x0040] = 0xFFFFFFFF;
	nv->pfifo[0x0415] = 0x00000001;
	nv->pfifo[0x048C] = 0;
	nv->pfifo[0x04A0] = 0;
	nv->pfifo[0x0489] = 0x000F0078;
	nv->pfifo[0x0488] = 0x00000001;
	nv->pfifo[0x0480] = 0x00000001;
	nv->pfifo[0x0494] = 0x00000001;
	nv->pfifo[0x0495] = 0x00000001;
	nv->pfifo[0x0140] = 0x00000001;

	if (nv->arch >= 10) {
		if (nv->twoheads) {
			nv->pcrtc[0x0860/4] = nv->head;
			nv->pcrtc[0x2860/4] = nv->head2;
		}
		nv->pramdac[0x0404/4] |= (1 << 25);

		nv->pmc[0x8704/4] = 1;
		nv->pmc[0x8140/4] = 0;
		nv->pmc[0x8920/4] = 0;
		nv->pmc[0x8924/4] = 0;
		nv->pmc[0x8908/4] = vga->vmz - 1;
		nv->pmc[0x890C/4] = vga->vmz - 1;
		nv->pmc[0x1588/4] = 0;

		nv->pcrtc[0x0810/4] = nv->cursorconfig;
		nv->pcrtc[0x0830/4] = nv->displayV - 3;
		nv->pcrtc[0x0834/4] = nv->displayV - 1;

		if (nv->islcd) {
			if((nv->did & 0x0ff0) == 0x0110)
				nv->pramdac[0x0528/4] = nv->dither;
			else if (nv->twoheads)
				nv->pramdac[0x083C/4] = nv->dither;
			vgaxo(Crtx, 0x53, nv->timingH);
			vgaxo(Crtx, 0x54, nv->timingV);
			vgaxo(Crtx, 0x21, 0xFA);
		}
		vgaxo(Crtx, 0x41, nv->extra);
	}

	vgaxo(Crtx, 0x19, nv->repaint0);
	vgaxo(Crtx, 0x1A, nv->repaint1);
	vgaxo(Crtx, 0x25, nv->screen);
	vgaxo(Crtx, 0x28, nv->pixel);
	vgaxo(Crtx, 0x2D, nv->horiz);
	vgaxo(Crtx, 0x30, nv->cursor0);
	vgaxo(Crtx, 0x31, nv->cursor1);
	vgaxo(Crtx, 0x2F, nv->cursor2);
	vgaxo(Crtx, 0x39, nv->interlace);

	if (nv->islcd) {
		nv->pramdac[0x00000848/4] = nv->scale;
		nv->pramdac[0x00000828/4] = nv->crtcsync;
	} else {
		nv->pramdac[0x50C/4] = nv->pllsel;
		nv->pramdac[0x508/4] = nv->vpll;
		if (nv->twoheads)
			nv->pramdac[0x520/4] = nv->vpll2;
		if (nv->twostagepll) {
			nv->pramdac[0x578/4] = nv->vpllB;
			nv->pramdac[0x57C/4] = nv->vpll2B;
		}
	}
	nv->pramdac[0x00000600/4] = nv->general;

	nv->pcrtc[0x0140/4] = 0;
	nv->pcrtc[0x0100/4] = 1;

	ctlr->flag |= Fload;
}


static void
dump(Vga* vga, Ctlr* ctlr)
{
	Nvidia *nv;
	int m, n, p, f;
	double trouble;

	if((nv = vga->private) == 0)
		return;

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
	printitem(ctlr->name, "arch");
	Bprint(&stdout, " %d\n", nv->arch);
	printitem(ctlr->name, "did");
	Bprint(&stdout, " %.4ux\n", nv->did);
	printitem(ctlr->name, "repaint0");
	Bprint(&stdout, " %ux\n", nv->repaint0);
	printitem(ctlr->name, "repaint1");
	Bprint(&stdout, " %ux\n", nv->repaint1);
	printitem(ctlr->name, "screen");
	Bprint(&stdout, " %ux\n", nv->screen);
	printitem(ctlr->name, "pixel");
	Bprint(&stdout, " %ux\n", nv->pixel);
	printitem(ctlr->name, "horiz");
	Bprint(&stdout, " %ux\n", nv->horiz);
	printitem(ctlr->name, "cursor0");
	Bprint(&stdout, " %ux\n", nv->cursor0);
	printitem(ctlr->name, "cursor1");
	Bprint(&stdout, " %ux\n", nv->cursor1);
	printitem(ctlr->name, "cursor2");
	Bprint(&stdout, " %ux\n", nv->cursor2);
	printitem(ctlr->name, "interlace");
	Bprint(&stdout, " %ux\n", nv->interlace);
	printitem(ctlr->name, "extra");
	Bprint(&stdout, " %ux\n", nv->extra);
	printitem(ctlr->name, "crtcowner");
	Bprint(&stdout, " %ux\n", nv->crtcowner);
	printitem(ctlr->name, "timingH");
	Bprint(&stdout, " %ux\n", nv->timingH);
	printitem(ctlr->name, "timingV");
	Bprint(&stdout, " %ux\n", nv->timingV);
	printitem(ctlr->name, "vpll");
	Bprint(&stdout, " %lux\n", nv->vpll);
	printitem(ctlr->name, "vpllB");
	Bprint(&stdout, " %lux\n", nv->vpllB);
	printitem(ctlr->name, "vpll2");
	Bprint(&stdout, " %lux\n", nv->vpll2);
	printitem(ctlr->name, "vpll2B");
	Bprint(&stdout, " %lux\n", nv->vpll2B);
	printitem(ctlr->name, "pllsel");
	Bprint(&stdout, " %lux\n", nv->pllsel);
	printitem(ctlr->name, "general");
	Bprint(&stdout, " %lux\n", nv->general);
	printitem(ctlr->name, "scale");
	Bprint(&stdout, " %lux\n", nv->scale);
	printitem(ctlr->name, "config");
	Bprint(&stdout, " %lux\n", nv->config);
	printitem(ctlr->name, "head");
	Bprint(&stdout, " %lux\n", nv->head);
	printitem(ctlr->name, "head2");
	Bprint(&stdout, " %lux\n", nv->head2);
	printitem(ctlr->name, "cursorconfig");
	Bprint(&stdout, " %lux\n", nv->cursorconfig);
	printitem(ctlr->name, "dither");
	Bprint(&stdout, " %lux\n", nv->dither);
	printitem(ctlr->name, "crtcsync");
	Bprint(&stdout, " %lux\n", nv->crtcsync);
	printitem(ctlr->name, "islcd");
	Bprint(&stdout, " %d\n", nv->islcd);
	printitem(ctlr->name, "twoheads");
	Bprint(&stdout, " %d\n", nv->twoheads);
	printitem(ctlr->name, "twostagepll");
	Bprint(&stdout, " %d\n", nv->twostagepll);
	printitem(ctlr->name, "crtcnumber");
	Bprint(&stdout, " %d\n", nv->crtcnumber);

	printitem(ctlr->name, "fpwidth");
	Bprint(&stdout, " %d\n", nv->fpwidth);
	printitem(ctlr->name, "fpheight");
	Bprint(&stdout, " %d\n", nv->fpheight);

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
