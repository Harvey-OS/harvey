#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Intel 81x chipset family.
 */

typedef struct {
	Pcidev*	pci;
	ulong	mmio;
	ulong	clk[6];
	ulong	lcd[9];
	ulong	pixconf;
} I81x;

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int f, i;
	long m;
	ulong *rp;
	Pcidev *p;
	I81x *i81x;

	if(vga->private == nil){
		vga->private = alloc(sizeof(I81x));
		p = nil;
		while((p = pcimatch(p, 0x8086, 0)) != nil) {
			switch(p->did) {
			default:
				continue;
			case 0x7121:
			case 0x7123:
			case 0x7125:
			case 0x1102:
			case 0x1112:
			case 0x1132:
				vga->f[1] = 230000000;
				break;
			}
			break;
		}
		if(p == nil)
			error("%s: Intel 81x graphics function not found\n", ctlr->name);

		if((f = open("#v/vgactl", OWRITE)) < 0)
			error("%s: can't open vgactl\n", ctlr->name);
		if(write(f, "type i81x", 9) != 9)
			error("%s: can't set type\n", ctlr->name);
		close(f);
	
		if((m = segattach(0, "i81xmmio", 0, p->mem[1].size)) == -1)
			error("%s: can't attach mmio segment\n", ctlr->name);
	
		i81x = vga->private;
		i81x->pci = p;
		i81x->mmio = m;
	}
	i81x = vga->private;

	vga->vma = vga->vmz = i81x->pci->mem[0].size;
	ctlr->flag |= Hlinear;

	for(i = 0x30; i <= 0x82; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	for(i = 0x10; i <= 0x1f; i++)
		vga->graphics[i] = vgaxi(Grx, i);

	rp = (ulong*)(i81x->mmio+0x06000);
	for(i = 0; i < nelem(i81x->clk); i++)
		i81x->clk[i] = *rp++;
	rp = (ulong*)(i81x->mmio+0x60000);
	for(i = 0; i < nelem(i81x->lcd); i++)
		i81x->lcd[i] = *rp++;
	rp = (ulong*)(i81x->mmio+0x70008);
	i81x->pixconf = *rp;

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	I81x *i81x;

	i81x = vga->private;

	/*
	 * Pixel pipeline mode: 16/24bp bypasses palette,
	 * hw cursor enabled, hi-res mode, depth.
	 */
	i81x->pixconf = (1<<12)|(1<<0);
	switch(vga->mode->z) {
	case 8:
		i81x->pixconf |= (2<<16);
		break;
	case 16:
		i81x->pixconf |= (5<<16);
		break;
	case 24:
		i81x->pixconf |= (6<<16);
		break;
	case 32:
		i81x->pixconf |= (7<<16);
		break;
	default:
		error("%s: depth %d not supported\n", ctlr->name, vga->mode->z);
	}

	if(vga->linear && (ctlr->flag & Hlinear)) {
		vga->graphics[0x10] = 0x02;		/* enable linear mapping */
		ctlr->flag |= Ulinear;
	}

	/* always use extended vga mode */
	vga->crt[0x80] = 0x01;
	vga->crt[0x30] = ((vga->mode->vt-2)>>8) & 0x0F;
	vga->crt[0x31] = ((vga->mode->y-1)>>8)&0x0f;
	vga->crt[0x32] = (vga->mode->vrs>>8) & 0x0F;
	vga->crt[0x33] = (vga->mode->vrs>>8) & 0x0F;
	vga->crt[0x35] = (((vga->mode->ht>>3)-5)>>8) & 0x01;
	vga->crt[0x39] = (vga->mode->ehb>>9) & 0x01;
	vga->crt[0x41] = (vga->crt[0x13]>>8) & 0x0F;
	vga->crt[0x42] = 0x00;
	vga->crt[0x40] = 0x80;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int i;
	ulong *rp;
	I81x *i81x;

	i81x = vga->private;

	vgaxo(Grx, 0x10, vga->graphics[0x10]);
	for(i = 0x30; i <= 0x82; i++)
		vgaxo(Crtx, i, vga->crt[i]);
	vgaxo(Crtx, 0x13, vga->crt[0x13]);

	rp = (ulong*)(i81x->mmio+0x06000);
	for(i = 0; i < nelem(i81x->clk); i++)
		*rp++ = i81x->clk[i];
	rp = (ulong*)(i81x->mmio+0x60000);
	for(i = 0; i < nelem(i81x->lcd); i++)
		*rp++ = i81x->lcd[i];
	rp = (ulong*)(i81x->mmio+0x70008);
	*rp = i81x->pixconf;

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	Pcidev *p;
	I81x *i81x;
	char *name;

	name = ctlr->name;
	i81x = vga->private;

	printitem(name, "Crt30");
	for(i = 0x30; i <= 0x39; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt40");
	for(i = 0x40; i <= 0x42; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt70");
	for(i = 0x70; i <= 0x79; i++)
		printreg(vga->crt[i]);

	printitem(name, "Crt80");
	for(i = 0x80; i <= 0x82; i++)
		printreg(vga->crt[i]);

	printitem(name, "Graphics10");
	for(i = 0x10; i <= 0x1f; i++)
		printreg(vga->graphics[i]);

	printitem(name, "clk");
	for(i = 0; i < nelem(i81x->clk); i++)
		printreg(i81x->clk[i]);

	printitem(name, "lcd");
	for(i = 0; i < nelem(i81x->lcd); i++)
		printreg(i81x->lcd[i]);

	printitem(name, "pixconf");
	printreg(i81x->pixconf);

	p = i81x->pci;
	printitem(name, "mem[0]");
	Bprint(&stdout, "base %lux size %d\n", p->mem[0].bar & ~0x0F, p->mem[0].size);

	printitem(name, "mem[1]");
	Bprint(&stdout, "base %lux size %d\n", p->mem[1].bar & ~0x0F, p->mem[1].size);

}

Ctlr i81x = {
	"i81x",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr i81xhwgc = {
	"i81xhwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};
