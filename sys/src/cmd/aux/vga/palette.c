#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

static ulong
xnto32(uchar x, int n)
{
	int s;
	ulong y;

	x &= (1<<n)-1;
	y = 0;
	for(s = 32 - n; s > 0; s -= n)
		y |= x<<s;
	if(s < 0)
		y |= x>>(-s);
	return y;
}

static void
setcolour(uchar p[3], ulong r, ulong g, ulong b)
{
	p[Red] = r>>(32-6);
	p[Green] = g>>(32-6);
	p[Blue] = b>>(32-6);
}

/*
 * Vga colour palette.
 */
static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;

	vga->pixmask = vgai(Pixmask);
	vga->pstatus = vgai(Pstatus);
	vgao(PaddrR, 0x00);
	for(i = 0; i < Pcolours; i++){
		vga->palette[i][Red] = vgai(Pdata);
		vga->palette[i][Green] = vgai(Pdata);
		vga->palette[i][Blue] = vgai(Pdata);
	}

	ctlr->flag |= Fsnarf;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	int i;
	uchar *p;
	ulong x;

	memset(vga->palette, 0, sizeof(vga->palette));
	vga->pixmask = 0xFF;
	if(vga->mode->z == 8){
		for(i = 0; i < Pcolours; i++){
			p = vga->palette[i^0xFF];
			setcolour(p, xnto32(i>>5, 3), xnto32(i>>2, 3), xnto32(i, 2));
		}
		p =  vga->palette[0x55^0xFF];
		setcolour(p, xnto32(0x15, 6), xnto32(0x15, 6), xnto32(0x15, 6));
		p =  vga->palette[0xAA^0xFF];
		setcolour(p, xnto32(0x2A, 6), xnto32(0x2A, 6), xnto32(0x2A, 6));
		p =  vga->palette[0xFF^0xFF];
		setcolour(p, xnto32(0x3F, 6), xnto32(0x3F, 6), xnto32(0x3F, 6));
	}
	else for(i = 0; i < 16; i++){
		x = xnto32((i*63)/15, 6);
		setcolour(vga->palette[i^0xFF], x, x, x);
	}

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int i;

	vgao(Pixmask, vga->pixmask);
	vgao(PaddrW, 0x00);
	for(i = 0; i < Pcolours; i++){
		vgao(Pdata, vga->palette[i][Red]);
		vgao(Pdata, vga->palette[i][Green]);
		vgao(Pdata, vga->palette[i][Blue]);
	}

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;

	printitem(ctlr->name, "palette");
	for(i = 0; i < Pcolours; i++){
		if(i && (i%6) == 0)
			Bprint(&stdout, "\n%-20s", "");
		Bprint(&stdout, " %2.2X/%2.2X/%2.2X", vga->palette[i][Red],
			vga->palette[i][Green], vga->palette[i][Blue]);
	}
	Bprint(&stdout, "\n");
}

Ctlr palette = {
	"palette",			/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
