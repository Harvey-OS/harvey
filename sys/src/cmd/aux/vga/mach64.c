#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ATI Mach64. Some hope. Kind of like a Mach32.
 * No support for accelerator so can only do up to 1024x768.
 *
 * All ATI Extended Registers are addressed using the modified index
 *	index = (0x02<<6)|(index & 0x3F);
 * so registers 0x00->0x3F map to 0x80->0xBF, but we will only ever
 * look at a few in the range 0xA0->0xBF. In this way we can stash
 * them in the vga->crt[] array.
 */
enum {
	Configcntl	= 0x6AEC,	/* Configuration control */
	Configstat	= 0x72EC,	/* Configuration status */
	Memcntl		= 0x52EC,	/* Memory control */
	Scratch1	= 0x46EC,	/* Scratch Register (BIOS info) */
};

typedef struct {
	ulong	configcntl;
	ulong	configstat;
	ulong	memcntl;
	ulong	scratch1;
} Mach64;

/*
 * There are a number of possible clock generator chips for these
 * boards. We can divide any frequency by 2 (bit<6> of b8).
 */
typedef struct {
	ulong	frequency;
	uchar	be;			/* <4> - bit<3> of frequency index */
	uchar	b9;			/* <1> - bit<2> of frequency index */
	uchar	genmo;			/* <3:2> - bits <1:0> of frequency index */
} Pclk;

enum {
	Npclkx		= 16,		/* number of clock entries per table */
};

/*
 * ATI18811-0
 */
static Pclk ati188110[Npclkx] = {
	{  42950000, 0x00, 0x00, 0x00 },
	{  48770000, 0x00, 0x00, 0x04 },
	{  92400000, 0x00, 0x00, 0x08 },
	{  36000000, 0x00, 0x00, 0x0C },
	{  50350000, 0x00, 0x02, 0x00 },
	{  56640000, 0x00, 0x02, 0x04 },
	{ 	  0, 0x00, 0x02, 0x08 },
	{  44900000, 0x00, 0x02, 0x0C },
	{  30240000, 0x10, 0x00, 0x00 },
	{  32000000, 0x10, 0x00, 0x04 },
	{ 110000000, 0x10, 0x00, 0x08 },
	{  80000000, 0x10, 0x00, 0x0C },
	{  39910000, 0x10, 0x02, 0x00 },
	{  44900000, 0x10, 0x02, 0x04 },
	{  75000000, 0x10, 0x02, 0x08 },
	{  65000000, 0x10, 0x02, 0x0C },
};

/*
 * ATI18811-1, ATI18811-2
 * PCLK_TABLE = 0 in Mach64 speak.
 */
static Pclk ati188111[Npclkx] = {
	{ 100000000, 0x00, 0x00, 0x00 },
	{ 126000000, 0x00, 0x00, 0x04 },
	{  92400000, 0x00, 0x00, 0x08 },
	{  36000000, 0x00, 0x00, 0x0C },
	{  50350000, 0x00, 0x02, 0x00 },
	{  56640000, 0x00, 0x02, 0x04 },
	{ 	  0, 0x00, 0x02, 0x08 },
	{  44900000, 0x00, 0x02, 0x0C },
	{ 135000000, 0x10, 0x00, 0x00 },
	{  32000000, 0x10, 0x00, 0x04 },
	{ 110000000, 0x10, 0x00, 0x08 },
	{  80000000, 0x10, 0x00, 0x0C },
	{  39910000, 0x10, 0x02, 0x00 },
	{  44900000, 0x10, 0x02, 0x04 },
	{  75000000, 0x10, 0x02, 0x08 },
	{  65000000, 0x10, 0x02, 0x0C },
};

/*
 * ATI18818
 * The first four entries are programmable and the default
 * settings are either those below or those below divided by 2
 * (PCLK_TABLE = 1 and PCLK_TABLE = 2 respectively in Mach64
 * speak).
 */
static Pclk ati18818[Npclkx] = {
	{  50350000, 0x00, 0x00, 0x00 },
	{  56640000, 0x00, 0x00, 0x04 },
	{  63000000, 0x00, 0x00, 0x08 },
	{  72000000, 0x00, 0x00, 0x0C },
	{  40000000, 0x00, 0x02, 0x00 },
	{  44900000, 0x00, 0x02, 0x04 },
	{  49500000, 0x00, 0x02, 0x08 },
	{  50000000, 0x00, 0x02, 0x0C },
	{ 	  0, 0x10, 0x00, 0x00 },
	{ 110000000, 0x10, 0x00, 0x04 },
	{ 126000000, 0x10, 0x00, 0x08 },
	{ 135000000, 0x10, 0x00, 0x0C },
	{ 	  0, 0x10, 0x02, 0x00 },
	{  80000000, 0x10, 0x02, 0x04 },
	{  75000000, 0x10, 0x02, 0x08 },
	{  65000000, 0x10, 0x02, 0x0C },
};

static Pclk *pclkp;			/* which clock chip we are using */
static ulong atix;			/* index to extended regsiters */

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

	/*
	 * Set the I/O address and offset for the ATI
	 * extended registers to something we know about.
	 */
	if(atix == 0){
		outportw(Grx, (0xCE<<8)|0x50);
		outportw(Grx, (0x81<<8)|0x51);
		atix = 0x1CE;
	}

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
		vga->private = alloc(sizeof(Mach64));
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int i;
	Mach64 *mach64;

	atixinit(vga, ctlr);
	for(i = 0xA0; i < 0xC0; i++)
		vga->crt[i] = atixi(i);

	mach64 = vga->private;
	mach64->configcntl = inportl(Configcntl);
	mach64->configstat = inportl(Configstat);
	mach64->memcntl = inportl(Memcntl);
	mach64->scratch1 = inportl(Scratch1);

	/*
	 * Memory size.
	 */
	switch(mach64->memcntl & 0x07){

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

	case 4:
		vga->vmz = 6*1024*1024;
		break;

	case 5:
		vga->vmz = 8*1024*1024;
		break;
	}

	ctlr->flag |= Fsnarf;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	int f, divisor, index;

	mode = vga->mode;

	/*
	 * Must somehow determine which clock chip to use here.
	 * For now, punt and assume ATI18818.
	 */
	pclkp = ati18818;
	if(pclkp == 0)
		error("%s: can't determine clock chip\n", ctlr->name);

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	/*
	 * Find a clock frequency close to what we want.
	 * 'Close' is within 1MHz.
	 */
	for(divisor = 0, index = 0; index < Npclkx; index++, divisor = 0){
		divisor = 1;
		f = pclkp[index].frequency/divisor;
		if(f < vga->f[0]+1000000 && f >= vga->f[0]-1000000)
			break;

		divisor = 2;
		f /= divisor;
		if(f < vga->f[0]+1000000 && f >= vga->f[0]-1000000)
			break;
	}
	if(divisor == 0)
		error("%s: no suitable clock for %lud\n",
			ctlr->name, vga->f[0]);

	vga->d[0] = divisor;
	vga->i[0] = index;

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
	vga->crt[0xB8] &= 0x3F;
	vga->crt[0xB9] &= 0xFD;
	vga->crt[0xBE] &= 0xE5;

	if(vga->d[0] == 2)
		vga->crt[0xB8] |= 0x40;
	vga->crt[0xB9] |= pclkp[vga->i[0]].b9;
	vga->crt[0xBE] |= pclkp[vga->i[0]].be;
	vga->misc |= pclkp[vga->i[0]].genmo;

	if(vga->mode->interlace == 'v')
		vga->crt[0xBE] |= 0x02;

	/*
	 * Turn off 128Kb CPU address bit so
	 * we only have a 64Kb aperture at 0xA0000.
	 */
	vga->crt[0xBD] &= ~0x04;

	ctlr->type = mach32.name;

	/*
	 * The Mach64 can only address 1Mb in VGA mode
	 */
	vga->vmz = 1*1024*1024;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	/*
	 * We should probably do something here to make sure we that we
	 * have access to all the video memory through the 64Kb VGA aperture
	 * by disabling and linear aperture and memory boundary and then
	 * enabling the VGA controller.
	 * But for now, let's just assume it's ok, the Mach64 documentation
	 * is just as clear as the Mach32 documentation.
	 */
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
	Mach64 *mach64;

	printitem(ctlr->name, "ATIX");
	for(i = 0xA0; i < 0xC0; i++)
		printreg(vga->crt[i]);

	if((mach64 = vga->private) == 0)
		return;

	printitem(ctlr->name, "CONFIGCNTL");
	Bprint(&stdout, "%.8lux\n", mach64->configcntl);
	printitem(ctlr->name, "CONFIGSTAT");
	Bprint(&stdout, "%.8lux\n", mach64->configstat);
	printitem(ctlr->name, "MEMCNTL");
	Bprint(&stdout, "%.8lux\n", mach64->memcntl);
	printitem(ctlr->name, "SCRATCH1");
	Bprint(&stdout, "%.8lux\n", mach64->scratch1);
}

Ctlr mach64 = {
	"mach64",			/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};
