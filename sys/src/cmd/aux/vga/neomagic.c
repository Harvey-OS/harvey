#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

typedef struct {
	Pcidev*	pci;

	int	x;
	int	y;
} Neomagic;

enum {
	ExtCrtx = 0x19,
	MaxCRT=0x85,
	MaxGR=0xc7,
};

enum {
	GeneralLockReg = 0x0A,
	ExtCRTDispAddr = 0x0E,
	ExtCRTOffset = 0x0F,
	SysIfaceCntl1 = 0x10,
	SysIfaceCntl2 = 0x11,
	SingleAddrPage = 0x15,		/* not changed? */
	DualAddrPage = 0x16,		/* not changed? */
	PanelDispCntlReg1 = 0x20,
	PanelDispCntlReg2 = 0x25,
	PanelDispCntlReg3 = 0x30,
	PanelVertCenterReg1 = 0x28,
	PanelVertCenterReg2 = 0x29,
	PanelVertCenterReg3 = 0x2A,
	PanelVertCenterReg4 = 0x32,	/* not 2070 */
	PanelHorizCenterReg1 = 0x33,
	PanelHorizCenterReg2 = 0x34,
	PanelHorizCenterReg3 = 0x35,
	PanelHorizCenterReg4 = 0x36,	/* 2160, 2200, 2360 */
	PanelVertCenterReg5 = 0x37,	/* 2200, 2360 */
	PanelHorizCenterReg5 = 0x38,	/* 2200, 2360 */

	ExtColorModeSelect = 0x90,

	VerticalExt = 0x70,		/* 2200; iobase+4 */
};

static int crts[] = {
	0x1D, 0x1F, 0x21, 0x23, 0x25, 0x2F,
	/* also 40-59, 60-69, 70-MaxCRT */
	-1
};

/*
 * Neomagic driver (fake)
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Pcidev *p;
	Neomagic *nm;

	generic.snarf(vga, ctlr);

	outportw(Grx, 0x2609);	/* unlock neo registers */
	outportw(Grx, 0x0015);	/* reset bank */

	for(i=0; crts[i] >= 0; i++)
		vga->crt[crts[i]] = vgaxi(Crtx, crts[i]);
	for(i=0x40; i <= MaxCRT; i++)
		vga->crt[i] = vgaxi(Crtx, i);

	for(i=0x08; i<=0x3F; i++)
		vga->graphics[i] = vgaxi(Grx, i);
	for(i=0x70; i<=MaxGR; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	if(vga->private == nil){
		vga->private = alloc(sizeof(Neomagic));
		nm = vga->private;
		if((p = pcimatch(0, 0x10C8, 0)) == nil)
			error("%s: not found\n", ctlr->name);
		switch(p->did){
		case 0x0003:			/* MagicGraph 128 ZV */
			vga->f[1] = 80000000;
			vga->vmz = 2048*1024;
			vga->apz = 4*1024*1024;
			break;
		case 0x0083:			/* MagicGraph 128 ZV+ */
			vga->f[1] = 80000000;
			vga->vmz = 2048*1024;
			vga->apz = 4*1024*1024;
			break;
		case 0x0004:			/* MagicGraph 128 XD */
			vga->f[1] = 90000000;
			vga->vmz = 2048*1024;
			vga->apz = 16*1024*1024;
			break;
		case 0x0005:			/* MagicMedia 256 AV */
			vga->f[1] = 110000000;
			vga->vmz = 2560*1024;
			vga->apz = 16*1024*1024;
			break;
		case 0x0006:			/* MagicMedia 256 ZX */
			vga->f[1] = 110000000;
			vga->vmz = 4096*1024;
			vga->apz = 16*1024*1024;
			break;
		case 0x0016:			/* MagicMedia 256 XL+ */
			vga->f[1] = 110000000;
			/* Vaio VESA BIOS says 6080, but then hwgc doesn't work */
			vga->vmz = 4096*1024;
			vga->apz = 32*1024*1024;
			break;
		case 0x0001:			/* MagicGraph 128 */
		case 0x0002:			/* MagicGraph 128 V */
		default:
			error("%s: DID %4.4uX unsupported\n",
				ctlr->name, p->did);
		}
		nm->pci = p;
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Ulinear|Hlinear|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Neomagic *nm;
	int i, h, v, t;

	generic.init(vga, ctlr);

	nm = vga->private;
	switch((vga->graphics[0x20]>>3)&3){
	case 0:
		nm->x = 640;
		nm->y = 480;
		break;
	case 1:
		nm->x = 800;
		nm->y = 600;
		break;
	case 2:
		nm->x = 1024;
		nm->y = 768;
	case 3:
		nm->x = 1280;
		nm->y = 1024;
		break;
	}

	vga->crt[0x0C] = 0;	/* vga starting address (offset) */
	vga->crt[0x0D] = 0;
	vga->graphics[GeneralLockReg] = 0x01;	/* (internal or simultaneous) */
	vga->attribute[0x10] &= ~0x40;	/* 2x4 mode not right for neomagic */

	t = 2;		/* LCD only (0x01 for external) */
	switch(vga->mode->x){
	case 1280:
		t |= 0x60;
		break;
	case 1024:
		t |= 0x40;
		break;
	case 800:
		t |= 0x20;
		break;
	}
	if(0 && (nm->pci->did == 0x0005) || (nm->pci->did == 0x0006)){
		vga->graphics[PanelDispCntlReg1] &= 0x98;
		vga->graphics[PanelDispCntlReg1] |= (t & ~0x98);
	}
	else{
		vga->graphics[PanelDispCntlReg1] &= 0xDC;	/* save bits 7:6, 4:2 */
		vga->graphics[PanelDispCntlReg1] |= (t & ~0xDC);
	}

	vga->graphics[PanelDispCntlReg2] &= 0x38;
	vga->graphics[PanelDispCntlReg3] &= 0xEF;
	vga->graphics[PanelVertCenterReg1] = 0x00;
	vga->graphics[PanelVertCenterReg2] = 0x00;
	vga->graphics[PanelVertCenterReg3] = 0x00;
	vga->graphics[PanelVertCenterReg4] = 0x00;
	vga->graphics[PanelVertCenterReg5] = 0x00;
	vga->graphics[PanelHorizCenterReg1] = 0x00;
	vga->graphics[PanelHorizCenterReg2] = 0x00;
	vga->graphics[PanelHorizCenterReg3] = 0x00;
	vga->graphics[PanelHorizCenterReg4] = 0x00;
	vga->graphics[PanelHorizCenterReg5] = 0x00;
	if(vga->mode->x < nm->x){
		vga->graphics[PanelDispCntlReg2] |= 0x01;
		vga->graphics[PanelDispCntlReg3] |= 0x10;
		h = ((nm->x - vga->mode->x) >> 4) - 1;
		v = ((nm->y - vga->mode->y) >> 1) - 2;
		switch(vga->mode->x){
		case 640:
			vga->graphics[PanelHorizCenterReg1] = h;
			vga->graphics[PanelVertCenterReg3] = v;
			break;
		case 800:
			vga->graphics[PanelHorizCenterReg2] = h;
			vga->graphics[PanelVertCenterReg4] = v;
			break;
		case 1024:
			vga->graphics[PanelHorizCenterReg5] = h;
			vga->graphics[PanelVertCenterReg5] = v;
			break;
		}
	}

	vga->graphics[ExtCRTDispAddr] = 0x10;
	vga->graphics[SysIfaceCntl1] &= 0x0F;
	vga->graphics[SysIfaceCntl1] |= 0x30;
	vga->graphics[SysIfaceCntl2] = 0x40;	/* make sure MMIO is enabled */
	vga->graphics[SingleAddrPage] = 0x00;
	vga->graphics[DualAddrPage] = 0x00;
	vga->graphics[ExtCRTOffset] = 0x00;
	t = vga->graphics[ExtColorModeSelect] & 0x70;	/* colour mode extension */
	if(vga->mode->z == 8){
		t |= 0x11;
		vga->crt[0x13] = vga->mode->x/8;
		vga->graphics[ExtCRTOffset] = vga->mode->x>>11;
		vga->graphics[0x05] = 0x00;	/* linear addressing? */
		vga->crt[0x14] = 0x40;	/* double word mode but don't count by 4 */
	}
	else if(vga->mode->z == 16){
		t |= 0x13;
		vga->crt[0x13] = vga->mode->x/4;
		vga->graphics[0x05] = 0x00;	/* linear addressing? */
		vga->crt[0x14] = 0x40;	/* double word mode but don't count by 4 */
		vga->graphics[ExtCRTOffset] = vga->mode->x>>10;
		for(i = 0; i < Pcolours; i++){
			vga->palette[i][Red] = i<<1;
			vga->palette[i][Green] = i;
			vga->palette[i][Blue] = i<<1;
		}
	}
	else if(vga->mode->z == 24){
		t |= 0x14;
		vga->crt[0x13] = (vga->mode->x*3)/8;
//		vga->graphics[0x05] = 0x00;	/* linear addressing? */
		vga->crt[0x14] = 0x40;	/* double word mode but don't count by 4 */
		vga->graphics[ExtCRTOffset] = (vga->mode->x*3)>>11;
		for(i = 0; i < Pcolours; i++){
			vga->palette[i][Red] = i;
			vga->palette[i][Green] = i;
			vga->palette[i][Blue] = i;
		}
	}
	else
		error("depth %d not supported\n", vga->mode->z);
	vga->graphics[ExtColorModeSelect] = t;

	vga->misc |= 0x0C;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	vgaxo(Grx, GeneralLockReg, vga->graphics[GeneralLockReg]);
	vgaxo(Grx, ExtColorModeSelect, vga->graphics[ExtColorModeSelect]);
	vgaxo(Grx, PanelDispCntlReg2, vga->graphics[PanelDispCntlReg2] & 0x39);
	sleep(200);

	generic.load(vga, ctlr);

	vgaxo(Grx, ExtCRTDispAddr, vga->graphics[ExtCRTDispAddr]);
	vgaxo(Grx, ExtCRTOffset, vga->graphics[ExtCRTOffset] & 0x39);
	vgaxo(Grx, SysIfaceCntl1, vga->graphics[SysIfaceCntl1]);
	if(ctlr->flag & Ulinear)
		vga->graphics[SysIfaceCntl2] |= 0x80;
	vgaxo(Grx, SysIfaceCntl2, vga->graphics[SysIfaceCntl2]);
	vgaxo(Grx, SingleAddrPage, vga->graphics[SingleAddrPage]);
	vgaxo(Grx, DualAddrPage, vga->graphics[DualAddrPage]);
	vgaxo(Grx, PanelDispCntlReg1, vga->graphics[PanelDispCntlReg1]);
	vgaxo(Grx, PanelDispCntlReg2, vga->graphics[PanelDispCntlReg2]);
	vgaxo(Grx, PanelDispCntlReg3, vga->graphics[PanelDispCntlReg3]);
	vgaxo(Grx, PanelVertCenterReg1, vga->graphics[PanelVertCenterReg1]);
	vgaxo(Grx, PanelVertCenterReg2, vga->graphics[PanelVertCenterReg2]);
	vgaxo(Grx, PanelVertCenterReg3, vga->graphics[PanelVertCenterReg3]);
	vgaxo(Grx, PanelVertCenterReg4, vga->graphics[PanelVertCenterReg4]);
	vgaxo(Grx, PanelHorizCenterReg1, vga->graphics[PanelHorizCenterReg1]);
	vgaxo(Grx, PanelHorizCenterReg2, vga->graphics[PanelHorizCenterReg2]);
	vgaxo(Grx, PanelHorizCenterReg3, vga->graphics[PanelHorizCenterReg3]);
	vgaxo(Grx, PanelHorizCenterReg4, vga->graphics[PanelHorizCenterReg4]);
	vgaxo(Grx, PanelVertCenterReg5, vga->graphics[PanelVertCenterReg5]);
	vgaxo(Grx, PanelHorizCenterReg5, vga->graphics[PanelHorizCenterReg5]);

	if(vga->mode->z != 8)
		palette.load(vga, ctlr);
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	char buf[100];

	generic.dump(vga, ctlr);

	for(i = 0; crts[i] >= 0; i++){
		sprint(buf, "Crt%2.2uX", crts[i]);
		printitem(ctlr->name, buf);
		printreg(vga->crt[crts[i]]);
	}
	printitem(ctlr->name, "Crt40");
	for(i=0x40; i<=0x59; i++)
		printreg(vga->crt[i]);
	printitem(ctlr->name, "Crt60");
	for(i=0x60; i<=0x69; i++)
		printreg(vga->crt[i]);
	printitem(ctlr->name, "Crt70");
	for (i = 0x70; i <= MaxCRT; i++)
		printreg(vga->crt[i]);

	printitem(ctlr->name, "Gr08");
	for(i=0x08; i<=0x3F; i++)
		printreg(vga->graphics[i]);
	printitem(ctlr->name, "Gr70");
	for(i=0x70; i<=MaxGR; i++)
		printreg(vga->graphics[i]);
}

Ctlr neomagic = {
	"neomagic",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr neomagichwgc = {
	"neomagichwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
