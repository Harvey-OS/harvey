#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * Intel 81x chipset family.
 *   mem[0]: AGP aperture memory, 64MB for 810-DC100, from 0xF4000000
 *   mem[1]: GC Register mmio space, 512KB for 810-DC100, from 0xFF000000
 *   For the memory of David Hogan, died April 9, 2003,  who wrote this driver 
 *   first for LCD.
 *                   August 28, 2003 Kenji Okamoto
 */

typedef struct {
	Pcidev*	pci;
	uchar*	mmio;
	ulong	clk[6];
	ulong	lcd[9];
	ulong	pixconf;
} I81x;

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int f, i;
	uchar *mmio;
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
			case 0x7121:	/* Vanilla 82810 */
			case 0x7123:	/* 810-DC100, DELL OptiPlex GX100 */
			case 0x7125:	/* 82810E */
			case 0x1102:	/* 82815 FSB limited to 100MHz */
			case 0x1112:	/* 82815 no AGP */
			case 0x1132:	/* 82815 fully featured Solano */
			case 0x3577:	/* IBM R31 uses intel 830M chipset */
				vga->f[1] = 230000000;		/* MAX speed of internal DAC (Hz)*/
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

		mmio = segattach(0, "i81xmmio", 0, p->mem[1].size);
		if(mmio == (void*)-1)
			error("%s: can't attach mmio segment\n", ctlr->name);
	
		i81x = vga->private;
		i81x->pci = p;
		i81x->mmio = mmio;
	}
	i81x = vga->private;

	/* must give aperture memory size for frame buffer memory
		such as 64*1024*1024 */
	vga->vma = vga->vmz = i81x->pci->mem[0].size;
//	vga->vmz = 8*1024*1024;
	vga->apz = i81x->pci->mem[0].size;
	ctlr->flag |= Hlinear;

	vga->graphics[0x10] = vgaxi(Grx, 0x10);
	vga->attribute[0x11] = vgaxi(Attrx, 0x11);	/* overscan color */
	for(i=0; i < 0x19; i++)
		vga->crt[i] = vgaxi(Crtx, i);
	for(i=0x30; i <= 0x82; i++)		/* set CRT Controller Register (CR) */
		vga->crt[i] = vgaxi(Crtx, i);
	/* 0x06000: Clock Control Register base address (3 VCO frequency control) */
	rp = (ulong*)(i81x->mmio+0x06000);
	for(i = 0; i < nelem(i81x->clk); i++)
		i81x->clk[i] = *rp++;

	/* i830 CRTC registers (A) */
	rp = (ulong*)(i81x->mmio+0x60000);
	for(i = 0; i < nelem(i81x->lcd); i++)
		i81x->lcd[i] = *rp++;
	rp = (ulong*)(i81x->mmio+0x70008);	/* Pixel Pipeline Control register A */
	i81x->pixconf = *rp;

	ctlr->flag |= Fsnarf;
}

static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Foptions;
}

static void
i81xdclk(I81x *i81x, Vga *vga)		/* freq = MHz */
{
	int m, n, post, mtp, ntp;
	double md, freq, error;

	freq = vga->mode->deffrequency/1000000.0;
	if (freq == 0)
		sysfatal("i81xdclk: deffrequency %d becomes freq 0.0",
			vga->mode->deffrequency);
	post = log(600.0/freq)/log(2.0);

	for(ntp=3;;ntp++) {
		md = freq*(1<<post)/(24.0/(double)ntp)/4.0;
		mtp = (int)(md+0.5);
		if(mtp<3) mtp=3;
		error = 1.0-freq/(md/(ntp*(1<<post))*4*24.0);
		if((fabs(error) < 0.001) || ((ntp > 30) && (fabs(error) < 0.005)))
			break;
	}
	m = vga->m[1] = mtp-2;
	n = vga->n[1] = ntp-2;
	vga->r[1] = post;
	i81x->clk[2] = ((n & 0x3FF)<<16) | (m & 0x3FF);
	i81x->clk[4] = (i81x->clk[4] & ~0x700000) | ((post & 0x07)<<20);
	vga->mode->frequency = (m+2)/((n+2)*(1<<post))*4*24*1000000;
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	I81x *i81x;
	int vt, vde, vrs, vre;
	ulong *rp;

	i81x = vga->private;

	/* <<TODO>>
	    i81x->clk[3]: LCD_CLKD: 0x0600c~0x0600f, default=00030013h
			(VCO N-divisor=03h, M-divisor=13h)
	    i81x->clk[4]: DCLK_0DS: 0x06010~0x06013, Post value, default=40404040h means
		Post Divisor=16, VCO Loop divisor = 4xM for all clocks.
	    Display&LCD Clock Devisor Select Reg = 0x40404040 ==> (LCD)(Clock2)(Clock1)(Clock0)
	*/
	i81x->clk[0] = 0x00030013;
	i81x->clk[1] = 0x00100053;
	rp = (ulong*)i81x->mmio+0x6010;
	i81x->clk[4] = *rp;
	i81x->clk[4] |= 0x4040;
	vga->misc = vgai(MiscR);
	switch(vga->virtx) {
	case 640:	/* 640x480 DCLK_0D 25.175MHz dot clock */
		vga->misc &=  ~0x0A;
		break;
	case 720:	/* 720x480 DCLK_1D 28.322MHz dot clock */
		vga->misc = (vga->misc & ~0x08) | (1<<2);
		break;
	case 800:
	case 1024:
	case 1152:
	case 1280:
	case 1376:
		vga->misc = vga->misc | (2<<2) & ~0x02;		/* prohibit to access frame buffer */
		i81xdclk(i81x, vga);
		break;
	default:	/* for other higher resolution DCLK_2D */
		error("%s: Only 800, 1024, 1152, 1280, 1376 resolutions are supported\n", ctlr->name);
	}

	/*	<<TODO>>
		i830 LCD Controller, at i81x->mmio+0x60000
		i81x->lcd[0]: Horizontal Total Reg.		0x60000
		i81x->lcd[1]: Horizontal Blanking Reg.	0x60004
		i81x->lcd[2]: Horizontal Sync Reg. 		0x60008
		i81x->lcd[3]: Vertical Total Reg.		0x6000c
		i81x->lcd[4]: Vertical Blanking Reg.		0x60010
		i81x->lcd[5]: Vertical Sync Reg. 		0x60014
		i81x->lcd[6]: Pixel Pipeline A Sequencer Register Control(SRC,0~7)	0x6001c
		i81x->lcd[7]: BCLRPAT_A	0x60020
	         i81x->lcd[8]: 0
	*/
	/*
	 * Pixel pipeline control register 0x70008: 
	 *    16/24bp bypasses palette,
	 *    hw cursor enabled(1<<12), hi-res mode(1<<0), depth(16-19 bit)
	 *    8bit DAC enable (1<<15), don't wrap to 256kM memory of VGA(1<<1).
	 *    enable extended palette addressing (1<<8)
	*/
	i81x->pixconf = (1<<12)|(1<<0);
	i81x->pixconf &= 0xFFFFFBFF;	/* disable overscan color */
	switch(vga->mode->z) {		/* vga->mode->z: color depth */
	case 8:
		i81x->pixconf |= (2<<16);
		break;
	case 16:	/* (5:6:5 bit) */
		i81x->pixconf |= (5<<16);
		break;
	case 24:
		i81x->pixconf |= (6<<16);
		break;
	case 32:		/* not supported */
		i81x->pixconf |= (7<<16);
		break;
	default:
		error("%s: depth %d not supported\n", ctlr->name, vga->mode->z);
	}

	/* DON'T CARE of Sequencer Reg. */
	/* DON'T CARE of Attribute registers other than this */
	vga->attribute[0x11] = 0;		/* over scancolor = black */
	/* DON't CARE of graphics[1], [2], [3], [4], [5], [6], [7] and [8] value */
	if(vga->linear && (ctlr->flag & Hlinear)) {
	/* enable linear mapping, no VGA memory and no page mapping */
		vga->graphics[0x10] = 0x0A;
		ctlr->flag |= Ulinear;
	}

	vt = vga->mode->vt;
	vde = vga->virty;
	vrs = vga->mode->vrs;
	vre = vga->mode->vre+6;		/* shift 7 pixel up */

	if(vga->mode->interlace == 'v') {
		vt /= 2;
		vde /= 2;
		vrs /= 2;
		vre /= 2;
	}
		/* Reset Row scan */
	vga->crt[8] = 0;
/* Line Compare, bit 6 of crt[9], bit 4 of crt[7] and crt[0x18], should be
 *	vga->crt[9] = vgaxi(Crtx, 9) | ((vde>>9 & 1)<<6) & 0x7F;
 *	vga->crt[7] = vgaxi(Crtx, 7) | ((vde>>8 & 1)<<4);
 *	vga->crt[0x18] = vde & 0xFF;
 *      However, above values don't work!!  I don't know why.   K.Okamoto
 */
	vga->crt[9] = 0;		/* I don't know why ? */
	vga->crt[7] = 0;		/* I don't know why ? */
	vga->crt[0x18] = 0;		/* I don't know why ? */
/*	32 bits Start Address of frame buffer (AGP aperture memory)
	vga->crt[0x42] = MSB 8 bits of Start Address Register, extended high start address Reg.
	vga->crt[0x40] = higer 6 bits in 0~5 bits, and the MSB = 1, extebded start address Reg.
	vga->crt[0x0C] = Start Address High Register
	vga->crt[0x0D] = Start Address Low Register
	LSB 2 bits of Start Address are always 0
 */
	vga->crt[0x42] = vga->pci->mem[0].bar>>24 & 0xFF;
	vga->crt[0x40] = vga->pci->mem[0].bar>>18 & 0x3F | 0x80;
		/* Start Address High */
	vga->crt[0x0C] = vga->pci->mem[0].bar>>10 & 0xFF;
		/* Start Address Low */
	vga->crt[0x0D] = (vga->pci->mem[0].bar >>2 + 1)& 0xFF;
		/* Underline Location, Memory Mode, DON'T CARE THIS VALUE */
	vga->crt[0x14] = 0x0;
		/* CRT Mode Control */
	vga->crt[0x17] = 0x80;		/* CRT normal mode */
		/* Frame buffer memory offset  (memory amount for a line) */
		/* vga->crt[0x13] = lower 8 bits of Offset Register
			vga->crt[0x41] = MSB 4 bits, those value should be
			vga->crt[0x13] = (vga->virtx*(vga->mode->z>>3)/4) & 0xFF;
			vga->crt[0x41] = (vga->virtx*(vga->mode->z>>3)/4)>>8 & 0x0F;
		  However, those doesn't work properly  K.Okamoto
		*/
	vga->crt[0x41] = (vga->crt[0x13]>>8) & 0x0F;		//dhog

		/* Horizontal Total */
	vga->crt[0] = ((vga->mode->ht>>3)-6) & 0xFF;
		/* Extended Horizontal Total Time Reg (ht)  */
	vga->crt[0x35] = vga->mode->ht>>12 & 0x01;
//	vga->crt[0x35] = (((vga->mode->ht>>1)-5)>>8) & 0x01;	//dhog
		/* Horizontal Display Enable End == horizontal width */
	vga->crt[1] = (vga->virtx-1)>>3 & 0xFF;
		/* Horizontal Blanking Start */
	vga->crt[2] = ((vga->mode->shb>>3)-1) & 0xFF;
		/* Horizontal blanking End crt[39](0),crt[5](7),crt[3](4:0) */
	vga->crt[3] = (vga->mode->shb - vga->virtx)>>3 & 0x1F;
	vga->crt[5] = ((vga->mode->shb - vga->virtx)>>3 & 0x20) <<2;
	vga->crt[0x39] = ((vga->mode->shb - vga->virtx)>>3 & 0x40) >>6;
//	vga->crt[0x39] = (vga->mode->ehb>>9) & 0x01;		//dhog
		/* Horizontal Sync Start */
	vga->crt[4] = vga->mode->shb>>3 & 0xFF;
		/* Horizontal Sync End */
	vga->crt[5] |= vga->mode->ehb>>3 & 0x1F;
		/* Extended Vertical Total (vt) */
	vga->crt[6] = (vt - 2) & 0xFF;
	vga->crt[0x30] = (vt - 2)>>8 & 0x0F;
		/* Vertical Sync Period */
	vga->crt[0x11] = (vre - vrs - 2) & 0x0F;
		/* Vertical Blanking End  */
	vga->crt[0x16] = (vre - vrs) & 0xFF;
		/* Extended Vertical Display End (y) */
	vga->crt[0x12] = (vde-1) & 0xFF;
	vga->crt[0x31] = (vde-1)>>8 & 0x0f;
		/* Extended Vertical Sync Start (vrs)  */
	vga->crt[0x10] = (vrs-1) & 0xFF;
	vga->crt[0x32] = (vrs-1)>>8 & 0x0F;
		/* Extended Vertical Blanking Start (vrs) */
	vga->crt[0x15] = vrs & 0xFF;
	vga->crt[0x33] = vrs>>8 & 0x0F;

	if(vga->mode->interlace == 'v')
		vga->crt[0x70] = vrs | 0x80;
	else
		vga->crt[0x70] = 0;
	vga->crt[0x80] = 1;

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	int i;
	ulong *rp;
	I81x *i81x;
	char *p;

	i81x = vga->private;

	vgaxo(Attrx, 0x11, vga->attribute[0x11]);
	/* set the screen graphic mode */
	vgaxo(Crtx, 0x80, vga->crt[0x80]);
	vgaxo(Grx, 0x10, vga->graphics[0x10]);
	vgao(MiscW, vga->misc);
	for(i=0; i <= 0x18; i++)
		vgaxo(Crtx, i, vga->crt[i]);
	for(i=0x30; i <= 0x82; i++)
		vgaxo(Crtx, i, vga->crt[i]);
	vga->crt[0x40] |= 0x80;			/* set CR40, then set the MSB bit of it */
	vgaxo(Crtx, 0x40, vga->crt[0x40]);
	/* 0x06000 = offset of Vertical Clock Devisor VGA0 */
	rp = (ulong*)(i81x->mmio+0x06000);
	for(i=0; i < nelem(i81x->clk); i++)
		*rp++ = i81x->clk[i];
	rp = (ulong*)(i81x->mmio+0x60000);
	for(i = 0; i < nelem(i81x->lcd); i++)
		*rp++ = i81x->lcd[i];
	/* set cursor, graphic mode */
	rp = (ulong*)(i81x->mmio+0x70008);
	*rp = i81x->pixconf | (1<<8);

	p = (char*)(i81x->mmio+Pixmask);	/* DACMASK */
	*p = 0xff;
	p = (char*)(i81x->mmio+PaddrW);		/* DACWX */
	*p = 0x04;
	p = (char*)(i81x->mmio+Pdata);		/* DACDATA */
	*p = 0xff;
	*p = 0xff;
	*p = 0xff;
	*p = 0x00;
	*p = 0x00;
	*p = 0x00;
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
