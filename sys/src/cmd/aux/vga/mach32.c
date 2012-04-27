#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ATI Mach32. Some hope.
 * No support for accelerator so can only do up to 1024x768.
 *
 * All ATI Extended Registers are addressed using the modified index
 *	index = (0x02<<6)|(index & 0x3F);
 * so registers 0x00->0x3F map to 0x80->0xBF, but we will only ever
 * look at a few in the range 0xA0->0xBF. In this way we can stash
 * them in the vga->crt[] array.
 */
enum {
	Advfunc		= 0x4AE8,	/* Advanced Function Control Register */
	Clocksel	= 0x4AEE,	/* Clock Select Register */
	Misc		= 0x36EE,	/* Miscellaneous Register */
	Membndry	= 0x42EE,	/* Memory Boundary Register */
	Memcfg		= 0x5EEE,	/* Memory Control Register */
};

typedef struct {
	ushort	advfunc;
	ushort	clocksel;
	ushort	misc;
	ushort	membndry;
	ushort	memcfg;
} Mach32;

/*
 * There are a number of possible clock generator chips for these
 * boards, and I don't know how to find out which is installed, other
 * than by looking at the board. So, pick a subset that will work for
 * all.
 */
typedef struct {
	ulong	frequency;
	uchar	b8;			/* <6> - divide by 2 */
	uchar	b9;			/* <1> - bit <3> of frequency index */
	uchar	be;			/* <4> - bit <2> of frequency index */
	uchar	misc;			/* <3:2> - bits <1:0> of frequency index */
} Clock;

static Clock clocks[] = {
	{  VgaFreq0,	0x40, 0x02, 0x00, 0x00, },
	{  32000000,	0x00, 0x00, 0x10, 0x04, },
	{  40000000,	0x00, 0x02, 0x10, 0x00, },
	{  44900000,	0x00, 0x02, 0x00, 0x0C, },
	{  65000000,	0x00, 0x02, 0x10, 0x0C, },
	{  75000000,	0x00, 0x02, 0x10, 0x08, },
	{	  0, },
};
	
static ulong atix;

static uchar
atixi(uchar index)
{
	outportb(atix, index);
	return inportb(atix+1);
}

static void
atixo(uchar index, uchar data)
{
	outportw(atix, (data<<8)|index);
}

static void
atixinit(Vga* vga, Ctlr*)
{
	uchar b;
	Mach32 *mach32;

	/*
	 * We could try to read in a part of the BIOS and try to determine
	 * the extended register address, but since we can't determine the offset value,
	 * we'll just have to assume the defaults all round.
	 */
	atix = 0x1CE;

	/*
	 * Unlock the ATI Extended Registers.
	 * We leave them unlocked from now on.
	 * Why does this chip have so many
	 * lock bits?
	 */
	if((b = atixi(0xB8)) & 0x3F)
		atixo(0xB8, b & 0xC0);
	b = atixi(0xAB);
	atixo(0xAB, b & ~0x18);
	atixo(0xB4, 0x00);
	b = atixi(0xB9);
	atixo(0xB9, b & ~0x80);
	b = atixi(0xBE);
	atixo(0xBE, b|0x09);

	if(vga->private == 0)
		vga->private = alloc(sizeof(mach32));
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Mach32 *mach32;

	atixinit(vga, ctlr);
	for(i = 0xA0; i < 0xC0; i++)
		vga->crt[i] = atixi(i);

	mach32 = vga->private;
	mach32->advfunc = inportw(Advfunc);
	mach32->clocksel = inportw(Clocksel);
	mach32->misc = inportw(Misc);
	mach32->membndry = inportw(Membndry);
	mach32->memcfg = inportw(Memcfg);

	/*
	 * Memory size.
	 */
	switch((mach32->misc>>2) & 0x03){

	case 0:
		vga->vmz = 512*1024;
		break;

	case 1:
		vga->vmz = 1024*1024;
		break;

	case 2:
		vga->vmz = 2*1024*1024;
		break;

	case 3:
		vga->vmz = 4*1024*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Foptions;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Clock *clockp;
	Mode *mode;

	mode = vga->mode;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	for(clockp = clocks; clockp->frequency; clockp++){
		if(clockp->frequency > vga->f[0]+100000)
			continue;
		if(clockp->frequency > vga->f[0]-100000)
			break;
	}
	if(clockp->frequency == 0)
		error("%s: no suitable clock for %lud\n",
			ctlr->name, vga->f[0]);

	vga->crt[0xB0] &= 0xDA;
	vga->crt[0xB1] &= 0x87;
	vga->crt[0xB5] &= 0x7E;
	vga->crt[0xB6] &= 0xE2;
	vga->crt[0xB3] &= 0xAF;
	vga->crt[0xA6] &= 0xFE;
	vga->crt[0xA7] &= 0xF4;

	/*
	 * 256-colour linear addressing.
	 */
	if(mode->z == 8){
		vga->graphics[0x05] = 0x00;
		vga->attribute[0x10] &= ~0x40;
		vga->crt[0x13] = (mode->x/8)/2;
		vga->crt[0x14] = 0x00;
		vga->crt[0x17] = 0xE3;

		vga->crt[0xB0] |= 0x20;
		vga->crt[0xB6] |= 0x04;
	}
	vga->attribute[0x11] = 0x00;
	vga->crt[0xB6] |= 0x01;
	vga->crt[0xBE] &= ~0x04;

	/*
	 * Do the clock index bits.
	 */
	vga->crt[0xB9] &= 0xFD;
	vga->crt[0xB8] &= 0x3F;
	vga->crt[0xBE] &= 0xE5;

	vga->crt[0xB8] |= clockp->b8;
	vga->crt[0xB9] |= clockp->b9;
	vga->crt[0xBE] |= clockp->be;
	vga->misc |= clockp->misc;

	if(vga->mode->interlace == 'v')
		vga->crt[0xBE] |= 0x02;

	/*
	 * Turn off 128Kb CPU address bit so
	 * we only have a 64Kb aperture at 0xA0000.
	 */
	vga->crt[0xBD] &= ~0x04;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	ushort x;

	/*
	 * Make sure we are in VGA mode,
	 * and that we have access to all the video memory through
	 * the 64Kb VGA aperture by disabling and linear aperture
	 * and memory boundary.
	 */
	outportw(Clocksel, 0x0000);
	x = inportw(Memcfg) & ~0x0003;
	outportw(Memcfg, x);
	outportw(Membndry, 0x0000);

	atixo(0xB0, vga->crt[0xB0]);
	atixo(0xB1, vga->crt[0xB1]);
	atixo(0xB5, vga->crt[0xB5]);
	atixo(0xB6, vga->crt[0xB6]);
	atixo(0xB3, vga->crt[0xB3]);
	atixo(0xA6, vga->crt[0xA6]);
	atixo(0xA7, vga->crt[0xA7]);
	atixo(0xB8, vga->crt[0xB8]);
	atixo(0xB9, vga->crt[0xB9]);
	atixo(0xBE, vga->crt[0xBE]);
	vgao(MiscW, vga->misc);

	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	Mach32 *mach32;

	printitem(ctlr->name, "ATIX");
	for(i = 0xA0; i < 0xC0; i++)
		printreg(vga->crt[i]);

	if((mach32 = vga->private) == 0)
		return;

	printitem(ctlr->name, "ADVFUNC");
	Bprint(&stdout, "%.4ux\n", mach32->advfunc);
	printitem(ctlr->name, "CLOCKSEL");
	Bprint(&stdout, "%.4ux\n", mach32->clocksel);
	printitem(ctlr->name, "MISC");
	Bprint(&stdout, "%.4ux\n", mach32->misc);
	printitem(ctlr->name, "MEMBNDRY");
	Bprint(&stdout, "%.4ux\n", mach32->membndry);
	printitem(ctlr->name, "MEMCFG");
	Bprint(&stdout, "%.4ux\n", mach32->memcfg);
}

Ctlr mach32 = {
	"mach32",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
