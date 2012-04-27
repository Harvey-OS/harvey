#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

/*
 * ATI Mach64 family.
 */
enum {
	HTotalDisp,
	HSyncStrtWid,
	VTotalDisp,
	VSyncStrtWid,
	VlineCrntVline,
	OffPitch,
	IntCntl,
	CrtcGenCntl,

	OvrClr,
	OvrWidLR,
	OvrWidTB,

	CurClr0,
	CurClr1,
	CurOffset,
	CurHVposn,
	CurHVoff,

	ScratchReg0,
	ScratchReg1,	/* Scratch Register (BIOS info) */
	ClockCntl,
	BusCntl,
	MemCntl,
	ExtMemCntl,
	MemVgaWpSel,
	MemVgaRpSel,
	DacRegs,
	DacCntl,
	GenTestCntl,
	ConfigCntl,	/* Configuration control */
	ConfigChipId,
	ConfigStat0,	/* Configuration status 0 */
	ConfigStat1,	/* Configuration status 1 */
	ConfigStat2,
	DspConfig,	/* Rage */
	DspOnOff,	/* Rage */

	DpBkgdClr,
	DpChainMsk,
	DpFrgdClr,
	DpMix,
	DpPixWidth,
	DpSrc,
	DpWriteMsk,

	LcdIndex,
	LcdData,

	Nreg,

	TvIndex = 0x1D,
	TvData = 0x27,

	LCD_ConfigPanel = 0,
	LCD_GenCtrl,
	LCD_DstnCntl,
	LCD_HfbPitchAddr,
	LCD_HorzStretch,
	LCD_VertStretch,
	LCD_ExtVertStretch,
	LCD_LtGio,
	LCD_PowerMngmnt,
	LCD_ZvgPio,
	Nlcd,
};

static char* iorname[Nreg] = {
	"HTotalDisp",
	"HSyncStrtWid",
	"VTotalDisp",
	"VSyncStrtWid",
	"VlineCrntVline",
	"OffPitch",
	"IntCntl",
	"CrtcGenCntl",

	"OvrClr",
	"OvrWidLR",
	"OvrWidTB",

	"CurClr0",
	"CurClr1",
	"CurOffset",
	"CurHVposn",
	"CurHVoff",

	"ScratchReg0",
	"ScratchReg1",
	"ClockCntl",
	"BusCntl",
	"MemCntl",
	"ExtMemCntl",
	"MemVgaWpSel",
	"MemVgaRpSel",
	"DacRegs",
	"DacCntl",
	"GenTestCntl",
	"ConfigCntl",
	"ConfigChipId",
	"ConfigStat0",
	"ConfigStat1",
	"ConfigStat2",
	"DspConfig",
	"DspOnOff",

	"DpBkgdClr",
	"DpChainMsk",
	"DpFrgdClr",
	"DpMix",
	"DpPixWidth",
	"DpSrc",
	"DpWriteMsk",

	"LcdIndex",
	"LcdData",	
};

static char* lcdname[Nlcd] = {
	"LCD ConfigPanel",
	"LCD GenCntl",
	"LCD DstnCntl",
	"LCD HfbPitchAddr",
	"LCD HorzStretch",
	"LCD VertStretch",
	"LCD ExtVertStretch",
	"LCD LtGio",
	"LCD PowerMngmnt",
	"LCD ZvgPio"
};

/*
 * Crummy hack: all io register offsets
 * here get IOREG or'ed in, so that we can
 * tell the difference between an uninitialized
 * array entry and HTotalDisp.
 */
enum {
	IOREG = 0x10000,
};
static ushort ioregs[Nreg] = {
 [HTotalDisp]		IOREG|0x0000,
 [HSyncStrtWid]	IOREG|0x0100,
 [VTotalDisp]		IOREG|0x0200,
 [VSyncStrtWid]		IOREG|0x0300,
 [VlineCrntVline]	IOREG|0x0400,
 [OffPitch]			IOREG|0x0500,
 [IntCntl]			IOREG|0x0600,
 [CrtcGenCntl]		IOREG|0x0700,
 [OvrClr]			IOREG|0x0800,
 [OvrWidLR]		IOREG|0x0900,
 [OvrWidTB]		IOREG|0x0A00,
 [CurClr0]			IOREG|0x0B00,
 [CurClr1]			IOREG|0x0C00,
 [CurOffset]		IOREG|0x0D00,
 [CurHVposn]		IOREG|0x0E00,
 [CurHVoff]		IOREG|0x0F00,
 [ScratchReg0]		IOREG|0x1000,
 [ScratchReg1]		IOREG|0x1100,
 [ClockCntl]		IOREG|0x1200,
 [BusCntl]			IOREG|0x1300,
 [MemCntl]		IOREG|0x1400,
 [MemVgaWpSel]	IOREG|0x1500,
 [MemVgaRpSel]	IOREG|0x1600,
 [DacRegs]		IOREG|0x1700,
 [DacCntl]			IOREG|0x1800,
 [GenTestCntl]		IOREG|0x1900,
 [ConfigCntl]		IOREG|0x1A00,
 [ConfigChipId]		IOREG|0x1B00,
 [ConfigStat0]		IOREG|0x1C00,
 [ConfigStat1]		IOREG|0x1D00,
/* [GpIo]				IOREG|0x1E00, */
/* [HTotalDisp]			IOREG|0x1F00, 	duplicate, says XFree86 */
};

static ushort pciregs[Nreg] = {
  [HTotalDisp]		0x00,
  [HSyncStrtWid]	0x01,
  [VTotalDisp]		0x02,
  [VSyncStrtWid]        0x03,
  [VlineCrntVline]      0x04,
  [OffPitch]		0x05,
  [IntCntl]		0x06,
  [CrtcGenCntl]		0x07,
  [DspConfig]		0x08,
  [DspOnOff]		0x09,
  [OvrClr]		0x10,
  [OvrWidLR]		0x11,
  [OvrWidTB]		0x12,
  [CurClr0]		0x18,
  [CurClr1]		0x19,
  [CurOffset]		0x1A,
  [CurHVposn]		0x1B,
  [CurHVoff]		0x1C,
  [ScratchReg0]		0x20,
  [ScratchReg1]		0x21,
  [ClockCntl]		0x24,
  [BusCntl]		0x28,
  [LcdIndex]		0x29,
  [LcdData]		0x2A,
  [ExtMemCntl]		0x2B,
  [MemCntl]		0x2C,
  [MemVgaWpSel]		0x2D,
  [MemVgaRpSel]		0x2E,
  [DacRegs]		0x30,
  [DacCntl]		0x31,
  [GenTestCntl]		0x34,
  [ConfigCntl]		0x37,
  [ConfigChipId]	0x38,
  [ConfigStat0]		0x39,
  [ConfigStat1]		0x25,	/* rsc: was 0x3A, but that's not what the LT manual says */
  [ConfigStat2]		0x26,
  [DpBkgdClr]		0xB0,
  [DpChainMsk]		0xB3,
  [DpFrgdClr]		0xB1,
  [DpMix]		0xB5,
  [DpPixWidth]		0xB4,
  [DpSrc]		0xB6,
  [DpWriteMsk]		0xB2,
};

enum {
	PLLm		= 0x02,
	PLLp		= 0x06,
	PLLn0		= 0x07,
	PLLn1		= 0x08,
	PLLn2		= 0x09,
	PLLn3		= 0x0A,
	PLLx            = 0x0B,         /* external divisor (Rage) */

	Npll		= 32,
	Ntv		= 1,		/* actually 256, but not used */
};

typedef struct Mach64xx	Mach64xx;
struct Mach64xx {
	ulong	io;
	Pcidev*	pci;
	int	bigmem;
	int	lcdon;
	int	lcdpanelid;

	ulong	reg[Nreg];
	ulong	lcd[Nlcd];
	ulong	tv[Ntv];
	uchar	pll[Npll];

	ulong	(*ior32)(Mach64xx*, int);
	void	(*iow32)(Mach64xx*, int, ulong);
};

static ulong
portior32(Mach64xx* mp, int r)
{
	if((ioregs[r] & IOREG) == 0)
		return ~0;

	return inportl(((ioregs[r] & ~IOREG)<<2)+mp->io);
}

static void
portiow32(Mach64xx* mp, int r, ulong l)
{
	if((ioregs[r] & IOREG) == 0)
		return;

	outportl(((ioregs[r] & ~IOREG)<<2)+mp->io, l);
}

static ulong
pciior32(Mach64xx* mp, int r)
{
	return inportl((pciregs[r]<<2)+mp->io);
}

static void
pciiow32(Mach64xx* mp, int r, ulong l)
{
	outportl((pciregs[r]<<2)+mp->io, l);
}

static uchar
pllr(Mach64xx* mp, int r)
{
	int io;

	if(mp->ior32 == portior32)
		io = ((ioregs[ClockCntl]&~IOREG)<<2)+mp->io;
	else
		io = (pciregs[ClockCntl]<<2)+mp->io;

	outportb(io+1, r<<2);
	return inportb(io+2);
}

static void
pllw(Mach64xx* mp, int r, uchar b)
{
	int io;

	if(mp->ior32 == portior32)
		io = ((ioregs[ClockCntl]&~IOREG)<<2)+mp->io;
	else
		io = (pciregs[ClockCntl]<<2)+mp->io;

	outportb(io+1, (r<<2)|0x02);
	outportb(io+2, b);
}

static ulong
lcdr32(Mach64xx *mp, ulong r)
{
	ulong or;

	or = mp->ior32(mp, LcdIndex);
	mp->iow32(mp, LcdIndex, (or&~0x0F) | (r&0x0F));
	return mp->ior32(mp, LcdData);
}

static void
lcdw32(Mach64xx *mp, ulong r, ulong v)
{
	ulong or;

	or = mp->ior32(mp, LcdIndex);
	mp->iow32(mp, LcdIndex, (or&~0x0F) | (r&0x0F));
	mp->iow32(mp, LcdData, v);
}

static ulong
tvr32(Mach64xx *mp, ulong r)
{
	outportb(mp->io+(TvIndex<<2), r&0x0F);
	return inportl(mp->io+(TvData<<2));
}

static void
tvw32(Mach64xx *mp, ulong r, ulong v)
{
	outportb(mp->io+(TvIndex<<2), r&0x0F);
	outportl(mp->io+(TvData<<2), v);
}

static int smallmem[] = {
	   512*1024,	  1024*1024,	 2*1024*1024,	 4*1024*1024,
	6*1024*1024,	8*1024*1024,	12*1024*1024,	16*1024*1024,
};

static int bigmem[] = {
	    512*1024,	  2*512*1024,	  3*512*1024,	  4*512*1024,
	  5*512*1024,	  6*512*1024,	  7*512*1024,	  8*512*1024,
	 5*1024*1024,	 6*1024*1024,	 7*1024*1024,	 8*1024*1024,
	10*1024*1024,	12*1024*1024,	14*1024*1024,	16*1024*1024,
};

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	Mach64xx *mp;
	int i;
	ulong v;

	if(vga->private == nil){
		vga->private = alloc(sizeof(Mach64xx));
		mp = vga->private;
		mp->io = 0x2EC;
		mp->ior32 = portior32;
		mp->iow32 = portiow32;
		mp->pci = pcimatch(0, 0x1002, 0);
		if (mp->pci) {
			if(v = mp->pci->mem[1].bar & ~0x3) {
				mp->io = v;
				mp->ior32 = pciior32;
				mp->iow32 = pciiow32;
			}
		}
	}

	mp = vga->private;
	for(i = 0; i < Nreg; i++)
		mp->reg[i] = mp->ior32(mp, i);

	for(i = 0; i < Npll; i++)
		mp->pll[i] = pllr(mp, i);

	switch(mp->reg[ConfigChipId] & 0xFFFF){
	default:
		mp->lcdpanelid = 0;
		break;
	case ('L'<<8)|'B':		/* 4C42: Rage LTPro AGP */
	case ('L'<<8)|'I':		/* 4C49: Rage 3D LTPro */
	case ('L'<<8)|'M':		/* 4C4D: Rage Mobility */
	case ('L'<<8)|'P':		/* 4C50: Rage 3D LTPro */
		for(i = 0; i < Nlcd; i++)
			mp->lcd[i] = lcdr32(mp, i);
		if(mp->lcd[LCD_GenCtrl] & 0x02)
			mp->lcdon = 1;
		mp->lcdpanelid = ((mp->reg[ConfigStat2]>>14) & 0x1F);
		break;
	}

	/*
	 * Check which memory size map we are using.
	 */
	mp->bigmem = 0;
	switch(mp->reg[ConfigChipId] & 0xFFFF){
		case ('G'<<8)|'B':	/* 4742: 264GT PRO */
		case ('G'<<8)|'D':	/* 4744: 264GT PRO */
		case ('G'<<8)|'I':	/* 4749: 264GT PRO */
		case ('G'<<8)|'M':	/* 474D: Rage XL */
		case ('G'<<8)|'P':	/* 4750: 264GT PRO */
		case ('G'<<8)|'Q':	/* 4751: 264GT PRO */
		case ('G'<<8)|'R':	/* 4752: */
		case ('G'<<8)|'U':	/* 4755: 264GT DVD */
		case ('G'<<8)|'V':	/* 4756: Rage2C */
		case ('G'<<8)|'Z':	/* 475A: Rage2C */
		case ('V'<<8)|'U':	/* 5655: 264VT3 */
		case ('V'<<8)|'V':	/* 5656: 264VT4 */
		case ('L'<<8)|'B':	/* 4C42: Rage LTPro AGP */
		case ('L'<<8)|'I':	/* 4C49: Rage 3D LTPro */
		case ('L'<<8)|'M':	/* 4C4D: Rage Mobility */
		case ('L'<<8)|'P':	/* 4C50: Rage 3D LTPro */
			mp->bigmem = 1;
			break;
		case ('G'<<8)|'T':	/* 4754: 264GT[B] */
		case ('V'<<8)|'T':	/* 5654: 264VT/GT/VTB */
			/*
			 * Only the VTB and GTB use the new memory encoding,
			 * and they are identified by a nonzero ChipVersion,
			 * apparently.
			 */
			if((mp->reg[ConfigChipId] >> 24) & 0x7)
				mp->bigmem = 1;
			break;
	}

	/*
	 * Memory size and aperture. It's recommended
	 * to use an 8Mb aperture on a 16Mb boundary.
	 */
	if(mp->bigmem)
		vga->vmz = bigmem[mp->reg[MemCntl] & 0x0F];
	else
		vga->vmz = smallmem[mp->reg[MemCntl] & 0x07];
	vga->vma = 16*1024*1024;

	switch(mp->reg[ConfigCntl]&0x3){
	case 0:
		vga->apz = 16*1024*1024;	/* empirical -rsc */
		break;
	case 1:
		vga->apz = 4*1024*1024;
		break;
	case 2:
		vga->apz = 8*1024*1024;
		break;
	case 3:
		vga->apz = 2*1024*1024;	/* empirical: mach64GX -rsc */
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
	int clk, m, n, p;
	double f, q;
	Mach64xx *mp;

	mp = vga->private;

	/*
	 * Don't compute clock timings for LCD panels.
	 * Just use what's already there.  We can't just use
	 * the frequency in the vgadb for this because 
	 * the frequency being programmed into the PLLs
	 * is not the frequency being used to compute the DSP
	 * settings.  The DSP-relevant frequency is the one
	 * we keep in /lib/vgadb.
	 */
	if(mp->lcdon){
		clk = mp->reg[ClockCntl] & 0x03;
		n = mp->pll[7+clk];
		p = (mp->pll[6]>>(clk*2)) & 0x03;
		p |= (mp->pll[11]>>(2+clk)) & 0x04;
		switch(p){
		case 0:
		case 1:
		case 2:
		case 3:
			p = 1<<p;
			break;
		case 4+0:
			p = 3;
			break;
		case 4+2:
			p = 6;
			break;
		case 4+3:
			p = 12;
			break;

		default:
		case 4+1:
			p = -1;
			break;
		}
		m = mp->pll[PLLm];
		f = (2.0*RefFreq*n)/(m*p) + 0.5;

		vga->m[0] = m;
		vga->p[0] = p;
		vga->n[0] = n;
		vga->f[0] = f;
		return;
	}

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	f = vga->f[0];

	/*
	 * To generate a specific output frequency, the reference (m),
	 * feedback (n), and post dividers (p) must be loaded with the
	 * appropriate divide-down ratios. In the following r is the
	 * XTALIN frequency (usually RefFreq) and t is the target frequency
	 * (vga->f).
	 *
	 * Use the maximum reference divider left by the BIOS for now,
	 * otherwise MCLK might be a concern. It can be calculated as
	 * follows:
	 * 			    Upper Limit of PLL Lock Range
	 *	Minimum PLLREFCLK = -----------------------------
	 *				      (2*255)
	 *
	 *				       XTALIN
	 *			m = Floor[-----------------]
	 *				  Minimum PLLREFCLK
	 *
	 * For an upper limit of 135MHz and XTALIN of 14.318MHz m
	 * would be 54.
	 */
	m = mp->pll[PLLm];
	vga->m[0] = m;

	/*
	 * The post divider may be 1, 2, 4 or 8 and is determined by
	 * calculating
	 *		 t*m
	 *	    q = -----
	 *		(2*r)
	 * and using the result to look-up p.
	 */
	q = (f*m)/(2*RefFreq);
	if(ctlr->flag&Uenhanced){
	  if(q > 255 || q < 10.6666666667)
		error("%s: vclk %lud out of range\n", ctlr->name, vga->f[0]);
	  if(q > 127.5)
		p = 1;
	  else if(q > 85)
		p = 2;
	  else if(q > 63.75)
		p = 3;
	  else if(q > 42.5)
		p = 4;
	  else if(q > 31.875)
		p = 6;
	  else if(q > 21.25)
		p = 8;
	  else
		p = 12;
	}else{
	  if(q > 255 || q < 16)
		error("%s: vclk %lud out of range\n", ctlr->name, vga->f[0]);
	  if(q >= 127.5)
		p = 1;
	  else if(q >= 63.5)
		p = 2;
	  else if(q >= 31.5)
		p = 4;
	  else
		p = 8;
	}
	vga->p[0] = p;

	/*
	 * The feedback divider should be kept in the range 0x80 to 0xFF
	 * and is found from
	 *	n = q*p
	 * rounded to the nearest whole number.
	 */
	vga->n[0] = (q*p)+0.5;
}

typedef struct Meminfo	Meminfo;
struct Meminfo {
	int latency;
	int latch;
	int trp;		/* filled in from card */
	int trcd;		/* filled in from card */
	int tcrd;		/* filled in from card */
	int tras;		/* filled in from card */
};

enum {
	Mdram,
	Medo,	
	Msdram,
	Mwram,
};

/*
 * The manuals and documentation are silent on which settings
 * to use for Mwdram, or how to tell which to use.
 */
static Meminfo meminfo[] = {
[Mdram]		{ 1, 0 },
[Medo]		{ 1, 2 },
[Msdram]	{ 3, 1 },
[Mwram]		{ 1, 3 },	/* non TYPE_A */
};

static ushort looplatencytab[2][2] = {
	{ 8, 6 },		/* DRAM: ≤1M, > 1M */
	{ 9, 8 },		/* SDRAM: ≤1M, > 1M */
};

static ushort cyclesperqwordtab[2][2] = {
	{ 3, 2 },		/* DRAM: ≤1M, > 1M */
	{ 2, 1 },		/* SDRAM: ≤1M, > 1M */
};

static int memtype[] = {
	-1,			/* disable memory access */
	Mdram,			/* basic DRAM */
	Medo,			/* EDO */
	Medo,			/* hyper page DRAM or EDO */
	Msdram,			/* SDRAM */
	Msdram,			/* SGRAM */
	Mwram,
	Mwram
};

/*
 * Calculate various memory parameters so that the card
 * fetches the right bytes at the right time.  I don't claim to
 * understand the actual calculations very well.
 *
 * This is remarkably useful on laptops, since knowledge of
 * x lets us find the frequency that the screen is really running
 * at, which is not necessarily in the VCLKs.
 */
static void
setdsp(Vga* vga, Ctlr*)
{
	Mach64xx *mp;
	Meminfo *mem;
	ushort table, memclk, memtyp;
	int i, prec, xprec, fprec;
	ulong t;
	double pw, x, fifosz, fifoon, fifooff;
	ushort dspon, dspoff;
	int afifosz, lat, ncycle, pfc, rcc;

	mp = vga->private;

	/*
	 * Get video ram configuration from BIOS and chip
	 */
	table = *(ushort*)readbios(sizeof table, 0xc0048);
	trace("rom table offset %uX\n", table);
	table = *(ushort*)readbios(sizeof table, 0xc0000+table+16);
	trace("freq table offset %uX\n", table);
	memclk = *(ushort*)readbios(sizeof memclk, 0xc0000+table+18);
	trace("memclk %ud\n", memclk);
	memtyp = memtype[mp->reg[ConfigStat0]&07];
	mem = &meminfo[memtyp];

	/*
	 * First we need to calculate x, the number of 
	 * XCLKs that one QWORD occupies in the display FIFO.
	 *
	 * For some reason, x gets stretched out if LCD stretching
	 * is turned on. 
	 */

	x = ((double)memclk*640000.0) /
		((double)vga->mode->frequency * (double)vga->mode->z);
	if(mp->lcd[LCD_HorzStretch] & (1<<31))
		x *= 4096.0 / (double)(mp->lcd[LCD_HorzStretch] & 0xFFFF);

	trace("memclk %d... x %f...", memclk, x);
	/*
	 * We have 14 bits to specify x in.  Decide where to
	 * put the decimal (err, binary) point by counting how
	 * many significant bits are in the integer portion of x.
	 */
	t = x;
	for(i=31; i>=0; i--)
		if(t & (1<<i))
			break;
	xprec = i+1;
	trace("t %lud... xprec %d...", t, xprec);

	/*
	 * The maximum FIFO size is the number of XCLKs per QWORD
	 * multiplied by 32, for some reason.  We have 11 bits to
	 * specify fifosz.
	 */
	fifosz = x * 32.0;
	trace("fifosz %f...", fifosz);
	t = fifosz;
	for(i=31; i>=0; i--)
		if(t & (1<<i))
			break;
	fprec = i+1;
	trace("fprec %d...", fprec);

	/*
	 * Precision is specified as 3 less than the number of bits
	 * in the integer part of x, and 5 less than the number of bits
	 * in the integer part of fifosz.
	 *
	 * It is bounded by zero and seven.
	 */
	prec = (xprec-3 > fprec-5) ? xprec-3 : fprec-5;
	if(prec < 0)
		prec = 0;
	if(prec > 7)
		prec = 7;

	xprec = prec+3;
	fprec = prec+5;
	trace("prec %d...", prec);

	/*
	 * Actual fifo size
	 */
	afifosz = (1<<fprec) / x;
	if(afifosz > 32)
		afifosz = 32;

	fifooff = ceil(x*(afifosz-1));

	/*
	 * I am suspicious of this table, lifted from ATI docs,
	 * because it doesn't agree with the Windows drivers.
	 * We always get 0x0A for lat+2 while Windows uses 0x08.
	 */
	lat = looplatencytab[memtyp > 1][vga->vmz > 1*1024*1024];
	trace("afifosz %d...fifooff %f...", afifosz, fifooff);

	/*
	 * Page fault clock
	 */
	t = mp->reg[MemCntl];
	mem->trp = (t>>8)&3;	/* RAS precharge time */
	mem->trcd = (t>>10)&3;	/* RAS to CAS delay */
	mem->tcrd = (t>>12)&1;	/* CAS to RAS delay */
	mem->tras = (t>>16)&7;	/* RAS low minimum pulse width */
	pfc = mem->trp + 1 + mem->trcd + 1 + mem->tcrd;
	trace("pfc %d...", pfc);

	/*
	 * Maximum random access cycle clock.
	 */
	ncycle = cyclesperqwordtab[memtyp > 1][vga->vmz > 1*1024*1024];
	rcc = mem->trp + 1 + mem->tras + 1;
	if(rcc < pfc+ncycle)
		rcc = pfc+ncycle;
	trace("rcc %d...", rcc);

	fifoon = (rcc > floor(x)) ? rcc : floor(x);
	fifoon += (3.0 * rcc) - 1 + pfc + ncycle;
	trace("fifoon %f...\n", fifoon);
	/*
	 * Now finally put the bits together.
	 * x is stored in a 14 bit field with xprec bits of integer.
	 */
	pw = x * (1<<(14-xprec));
	mp->reg[DspConfig] = (ulong)pw | (((lat+2)&0xF)<<16) | ((prec&7)<<20);

	/*
	 * These are stored in an 11 bit field with fprec bits of integer.
	 */
	dspon  = (ushort)fifoon << (11-fprec);
	dspoff = (ushort)fifooff << (11-fprec);
	mp->reg[DspOnOff] = ((dspon&0x7ff) << 16) | (dspoff&0x7ff);
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode *mode;
	Mach64xx *mp;
	int p, x, y;

	mode = vga->mode;
	if((mode->x > 640 || mode->y > 480) && mode->z == 1)
		error("%s: no support for 1-bit mode other than 640x480x1\n",
			ctlr->name);

	mp = vga->private;
	if(mode->z > 8 && mp->pci == nil)
		error("%s: no support for >8-bit color without PCI\n",
			ctlr->name);

	/*
	 * Check for Rage chip
	 */
	switch (mp->reg[ConfigChipId]&0xffff) {
		case ('G'<<8)|'B':	/* 4742: 264GT PRO */
		case ('G'<<8)|'D':	/* 4744: 264GT PRO */
		case ('G'<<8)|'I':	/* 4749: 264GT PRO */
		case ('G'<<8)|'M':	/* 474D: Rage XL */
		case ('G'<<8)|'P':	/* 4750: 264GT PRO */
		case ('G'<<8)|'Q':	/* 4751: 264GT PRO */
		case ('G'<<8)|'R':	/* 4752: */
		case ('G'<<8)|'U':	/* 4755: 264GT DVD */
		case ('G'<<8)|'V':	/* 4756: Rage2C */
		case ('G'<<8)|'Z':	/* 475A: Rage2C */
		case ('V'<<8)|'U':	/* 5655: 264VT3 */
		case ('V'<<8)|'V':	/* 5656: 264VT4 */
		case ('G'<<8)|'T':	/* 4754: 264GT[B] */
		case ('V'<<8)|'T':	/* 5654: 264VT/GT/VTB */
		case ('L'<<8)|'B':	/* 4C42: Rage LTPro AGP */
		case ('L'<<8)|'I':	/* 4C49: 264LT PRO */
		case ('L'<<8)|'M':	/* 4C4D: Rage Mobility */
		case ('L'<<8)|'P':	/* 4C50: 264LT PRO */
			ctlr->flag |= Uenhanced;
			break;
	}

	/*
	 * Always use VCLK2.
	 */
	clock(vga, ctlr);
	mp->pll[PLLn2] = vga->n[0];
	mp->pll[PLLp] &= ~(0x03<<(2*2));
	switch(vga->p[0]){
	case 1:
	case 3:
		p = 0;
		break;

	case 2:
		p = 1;
		break;

	case 4:
	case 6:
		p = 2;
		break;

	case 8:
	case 12:
		p = 3;
		break;

	default:
		p = 3;
		break;
	}
	mp->pll[PLLp] |= p<<(2*2);
	if ((1<<p) != vga->p[0])
		mp->pll[PLLx] |= 1<<(4+2);
	else
		mp->pll[PLLx] &= ~(1<<(4+2));
	mp->reg[ClockCntl] = 2;

	mp->reg[ConfigCntl] = 0;

	mp->reg[CrtcGenCntl] = 0x02000000|(mp->reg[CrtcGenCntl] & ~0x01400700);
	switch(mode->z){
	default:
	case 1:
		mp->reg[CrtcGenCntl] |= 0x00000100;
		mp->reg[DpPixWidth] = 0x00000000;
		break;
	case 8:
		mp->reg[CrtcGenCntl] |= 0x01000200;
		mp->reg[DpPixWidth] = 0x00020202;
		break;
	case 15:
		mp->reg[CrtcGenCntl] |= 0x01000300;
		mp->reg[DpPixWidth] = 0x00030303;
		break;
	case 16:
		mp->reg[CrtcGenCntl] |= 0x01000400;
		mp->reg[DpPixWidth] = 0x00040404;
		break;
	case 24:
		mp->reg[CrtcGenCntl] |= 0x01000500;
		mp->reg[DpPixWidth] = 0x00050505;
		break;
	case 32:
		mp->reg[CrtcGenCntl] |= 0x01000600;
		mp->reg[DpPixWidth] = 0x00060606;
		break;
	}

	mp->reg[HTotalDisp] = (((mode->x>>3)-1)<<16)|((mode->ht>>3)-1);
	mp->reg[HSyncStrtWid] = (((mode->ehs - mode->shs)>>3)<<16)
				|((mode->shs>>3)-1);
	if(mode->hsync == '-')
		mp->reg[HSyncStrtWid] |= 0x00200000;
	mp->reg[VTotalDisp] = ((mode->y-1)<<16)|(mode->vt-1);
	mp->reg[VSyncStrtWid] = ((mode->vre - mode->vrs)<<16)|(mode->vrs-1);
	if(mode->vsync == '-')
		mp->reg[VSyncStrtWid] |= 0x00200000;
	mp->reg[IntCntl] = 0;

	/*
	 * This used to set it to (mode->x/(8*2))<<22 for depths < 8,
	 * but from the manual that seems wrong to me.  -rsc
	 */
	mp->reg[OffPitch] = (vga->virtx/8)<<22;

	mp->reg[OvrClr] = Pblack;

	if(vga->linear && mode->z != 1)
		ctlr->flag |= Ulinear;

	/*
	 * Heuristic fiddling on LT PRO.
	 * Do this before setdsp so the stretching is right.
	 */
	if(mp->lcdon){
		/* use non-shadowed registers */
		mp->lcd[LCD_GenCtrl] &= ~0x00000404;
		mp->lcd[LCD_ConfigPanel] |= 0x00004000;

		mp->lcd[LCD_VertStretch] = 0;
		y = ((mp->lcd[LCD_ExtVertStretch]>>11) & 0x7FF)+1;
		if(mode->y < y){
			x = (mode->y*1024)/y;
			mp->lcd[LCD_VertStretch] = 0xC0000000|x;
		}
		mp->lcd[LCD_ExtVertStretch] &= ~0x00400400;

		/*
		 * The x value doesn't seem to be available on all
		 * chips so intuit it from the y value which seems to
		 * be reliable.
		 */
		mp->lcd[LCD_HorzStretch] &= ~0xC00000FF;
		x = (mp->lcd[LCD_HorzStretch]>>20) & 0xFF;
		if(x == 0){
			switch(y){
			default:
				break;
			case 480:
				x = 640;
				break;
			case 600:
				x = 800;
				break;
			case 768:
				x = 1024;
				break;
			case 1024:
				x = 1280;
				break;
			}
		}
		else
			x = (x+1)*8;
		if(mode->x < x){
			x = (mode->x*4096)/x;
			mp->lcd[LCD_HorzStretch] |= 0xC0000000|x;
		}
	}

	if(ctlr->flag&Uenhanced)
		setdsp(vga, ctlr);

	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	Mach64xx *mp;
	int i;

	mp = vga->private;

	/*
	 * Unlock the CRTC and LCD registers.
	 */
	mp->iow32(mp, CrtcGenCntl, mp->ior32(mp, CrtcGenCntl)&~0x00400000);
	if(mp->lcdon)
		lcdw32(mp, LCD_GenCtrl, mp->lcd[LCD_GenCtrl]|0x80000000);

	/*
	 * Always use an aperture on a 16Mb boundary.
	 */
	if(ctlr->flag & Ulinear)
		mp->reg[ConfigCntl] = ((vga->vmb/(4*1024*1024))<<4)|0x02;

	mp->iow32(mp, ConfigCntl, mp->reg[ConfigCntl]);

	mp->iow32(mp, GenTestCntl, 0);
	mp->iow32(mp, GenTestCntl, 0x100);

	if((ctlr->flag&Uenhanced) == 0)
	  mp->iow32(mp, MemCntl, mp->reg[MemCntl] & ~0x70000);
	mp->iow32(mp, BusCntl, mp->reg[BusCntl]);
	mp->iow32(mp, HTotalDisp, mp->reg[HTotalDisp]);
	mp->iow32(mp, HSyncStrtWid, mp->reg[HSyncStrtWid]);
	mp->iow32(mp, VTotalDisp, mp->reg[VTotalDisp]);
	mp->iow32(mp, VSyncStrtWid, mp->reg[VSyncStrtWid]);
	mp->iow32(mp, IntCntl, mp->reg[IntCntl]);
	mp->iow32(mp, OffPitch, mp->reg[OffPitch]);
	if(mp->lcdon){
		for(i=0; i<Nlcd; i++)
			lcdw32(mp, i, mp->lcd[i]);
	}

	mp->iow32(mp, GenTestCntl, mp->reg[GenTestCntl]);
	mp->iow32(mp, ConfigCntl, mp->reg[ConfigCntl]);
	mp->iow32(mp, CrtcGenCntl, mp->reg[CrtcGenCntl]);
	mp->iow32(mp, OvrClr, mp->reg[OvrClr]);
	mp->iow32(mp, OvrWidLR, mp->reg[OvrWidLR]);
	mp->iow32(mp, OvrWidTB, mp->reg[OvrWidTB]);
	if(ctlr->flag&Uenhanced){
	  mp->iow32(mp, DacRegs, mp->reg[DacRegs]);
	  mp->iow32(mp, DacCntl, mp->reg[DacCntl]);
	  mp->iow32(mp, CrtcGenCntl, mp->reg[CrtcGenCntl]&~0x02000000);
	  mp->iow32(mp, DspOnOff, mp->reg[DspOnOff]);
	  mp->iow32(mp, DspConfig, mp->reg[DspConfig]);
	  mp->iow32(mp, CrtcGenCntl, mp->reg[CrtcGenCntl]);
	  pllw(mp, PLLx, mp->pll[PLLx]);
	}
	pllw(mp, PLLn2, mp->pll[PLLn2]);
	pllw(mp, PLLp, mp->pll[PLLp]);
	pllw(mp, PLLn3, mp->pll[PLLn3]);

	mp->iow32(mp, ClockCntl, mp->reg[ClockCntl]);
	mp->iow32(mp, ClockCntl, 0x40|mp->reg[ClockCntl]);

	mp->iow32(mp, DpPixWidth, mp->reg[DpPixWidth]);

	if(vga->mode->z > 8){
		int sh, i;
		/*
		 * We need to initialize the palette, since the DACs use it
		 * in true color modes.  First see if the card supports an
		 * 8-bit DAC.
		 */
		mp->iow32(mp, DacCntl, mp->reg[DacCntl] | 0x100);
		if(mp->ior32(mp, DacCntl)&0x100){
			/* card appears to support it */
			vgactlw("palettedepth", "8");
			mp->reg[DacCntl] |= 0x100;
		}

		if(mp->reg[DacCntl] & 0x100)
			sh = 0;	/* 8-bit DAC */
		else
			sh = 2;	/* 6-bit DAC */

		for(i=0; i<256; i++)
			setpalette(i, i>>sh, i>>sh, i>>sh);
	}

	ctlr->flag |= Fload;
}

static void
pixelclock(Vga* vga, Ctlr* ctlr)
{
	Mach64xx *mp;
	ushort table, s;
	int memclk, ref_freq, ref_divider, min_freq, max_freq;
	int feedback, nmult, pd, post, value;
	int clock;

	/*
	 * Find the pixel clock from the BIOS and current
	 * settings. Lifted from the ATI-supplied example code.
	 * The clocks stored in the BIOS table are in kHz/10.
	 *
	 * This is the clock LCDs use in vgadb to set the DSP
	 * values.
	 */
	mp = vga->private;

	/*
	 * GetPLLInfo()
	 */
	table = *(ushort*)readbios(sizeof table, 0xc0048);
	trace("rom table offset %uX\n", table);
	table = *(ushort*)readbios(sizeof table, 0xc0000+table+16);
	trace("freq table offset %uX\n", table);
	s = *(ushort*)readbios(sizeof s, 0xc0000+table+18);
	memclk = s*10000;
	trace("memclk %ud\n", memclk);
	s = *(ushort*)readbios(sizeof s, 0xc0000+table+8);
	ref_freq = s*10000;
	trace("ref_freq %ud\n", ref_freq);
	s = *(ushort*)readbios(sizeof s, 0xc0000+table+10);
	ref_divider = s;
	trace("ref_divider %ud\n", ref_divider);
	s = *(ushort*)readbios(sizeof s, 0xc0000+table+2);
	min_freq = s*10000;
	trace("min_freq %ud\n", min_freq);
	s = *(ushort*)readbios(sizeof s, 0xc0000+table+4);
	max_freq = s*10000;
	trace("max_freq %ud\n", max_freq);

	/*
	 * GetDivider()
	 */
	pd = mp->pll[PLLp] & 0x03;
	value = (mp->pll[PLLx] & 0x10)>>2;
	trace("pd %uX value %uX (|%d)\n", pd, value, value|pd);
	value |= pd;
	post = 0;
	switch(value){
	case 0:
		post = 1;
		break;
	case 1:
		post = 2;
		break;
	case 2:
		post = 4;
		break;
	case 3:
		post = 8;
		break;
	case 4:
		post = 3;
		break;
	case 5:
		post = 0;
		break;
	case 6:
		post = 6;
		break;
	case 7:
		post = 12;
		break;
	}
	trace("post = %d\n", post);

	feedback = mp->pll[PLLn0];
	if(mp->pll[PLLx] & 0x08)
		nmult = 4;
	else
		nmult = 2;

	clock = (ref_freq/10000)*nmult*feedback;
	clock /= ref_divider*post;
	clock *= 10000;

	Bprint(&stdout, "%s pixel clock = %ud\n", ctlr->name, clock);
}

static void dumpmach64bios(Mach64xx*);

static void
dump(Vga* vga, Ctlr* ctlr)
{
	Mach64xx *mp;
	int i, m, n, p;
	double f;
	static int first = 1;

	if((mp = vga->private) == 0)
		return;

	Bprint(&stdout, "%s pci %p io %lux %s\n", ctlr->name,
		mp->pci, mp->io, mp->ior32 == pciior32 ? "pciregs" : "ioregs");
	if(mp->pci)
		Bprint(&stdout, "%s ccru %ux\n", ctlr->name, mp->pci->ccru);
	for(i = 0; i < Nreg; i++)
		Bprint(&stdout, "%s %-*s%.8luX\n",
			ctlr->name, 20, iorname[i], mp->reg[i]);

	printitem(ctlr->name, "PLL");
	for(i = 0; i < Npll; i++)
		printreg(mp->pll[i]);
	Bprint(&stdout, "\n");

	switch(mp->reg[ConfigChipId] & 0xFFFF){
	default:
		break;
	case ('L'<<8)|'B':		/* 4C42: Rage LTPro AGP */
	case ('L'<<8)|'I':		/* 4C49: Rage 3D LTPro */
	case ('L'<<8)|'M':		/* 4C4D: Rage Mobility */
	case ('L'<<8)|'P':		/* 4C50: Rage 3D LTPro */
		for(i = 0; i < Nlcd; i++)
			Bprint(&stdout, "%s %-*s%.8luX\n",
				ctlr->name, 20, lcdname[i], mp->lcd[i]);
		break;
	}

	/*
	 *     (2*r*n)
	 * f = -------
	 *	(m*p)
	 */
	m = mp->pll[2];
	for(i = 0; i < 4; i++){
		n = mp->pll[7+i];
		p = (mp->pll[6]>>(i*2)) & 0x03;
		p |= (mp->pll[11]>>(2+i)) & 0x04;
		switch(p){
		case 0:
		case 1:
		case 2:
		case 3:
			p = 1<<p;
			break;
		case 4+0:
			p = 3;
			break;
		case 4+2:
			p = 6;
			break;
		case 4+3:
			p = 12;
			break;

		default:
		case 4+1:
			p = -1;
			break;
		}
		if(m*p == 0)
			Bprint(&stdout, "unknown VCLK%d\n", i);
		else {
			f = (2.0*RefFreq*n)/(m*p) + 0.5;
			Bprint(&stdout, "%s VCLK%d\t%ud\n", ctlr->name, i, (int)f);
		}
	}

	pixelclock(vga, ctlr);

	if(first) {
		first = 0;
		dumpmach64bios(mp);
	}
}

enum {
	ClockFixed=0,
	ClockIcs2595,
	ClockStg1703,
	ClockCh8398,
	ClockInternal,
	ClockAtt20c408,
	ClockIbmrgb514
};

/*
 * mostly derived from the xfree86 probe routines.
 */
static void 
dumpmach64bios(Mach64xx *mp)
{
	int i, romtable, clocktable, freqtable, lcdtable, lcdpanel;
	uchar bios[0x10000];

	memmove(bios, readbios(sizeof bios, 0xC0000), sizeof bios);

	/* find magic string */
	for(i=0; i<1024; i++)
		if(strncmp((char*)bios+i, " 761295520", 10) == 0)
			break;

	if(i==1024) {
		Bprint(&stdout, "no ATI bios found\n");
		return;
	}

	/* this is horribly endian dependent.  sorry. */
	romtable = *(ushort*)(bios+0x48);
	if(romtable+0x12 > sizeof(bios)) {
		Bprint(&stdout, "couldn't find ATI rom table\n");
		return;
	}

	clocktable = *(ushort*)(bios+romtable+0x10);
	if(clocktable+0x0C > sizeof(bios)) {
		Bprint(&stdout, "couldn't find ATI clock table\n");
		return;
	}

	freqtable = *(ushort*)(bios+clocktable-2);
	if(freqtable+0x20 > sizeof(bios)) {
		Bprint(&stdout, "couldn't find ATI frequency table\n");
		return;
	}

	Bprint(&stdout, "ATI BIOS rom 0x%x freq 0x%x clock 0x%x\n", romtable, freqtable, clocktable);
	Bprint(&stdout, "clocks:");
	for(i=0; i<16; i++)
		Bprint(&stdout, " %d", *(ushort*)(bios+freqtable+2*i));
	Bprint(&stdout, "\n");

	Bprint(&stdout, "programmable clock: %d\n", bios[clocktable]);
	Bprint(&stdout, "clock to program: %d\n", bios[clocktable+6]);

	if(*(ushort*)(bios+clocktable+8) != 1430) {
		Bprint(&stdout, "reference numerator: %d\n", *(ushort*)(bios+clocktable+8)*10);
		Bprint(&stdout, "reference denominator: 1\n");
	} else {
		Bprint(&stdout, "default reference numerator: 157500\n");
		Bprint(&stdout, "default reference denominator: 11\n");
	}

	switch(bios[clocktable]) {
	case ClockIcs2595:
		Bprint(&stdout, "ics2595\n");
		Bprint(&stdout, "reference divider: %d\n", *(ushort*)(bios+clocktable+0x0A));
		break;
	case ClockStg1703:
		Bprint(&stdout, "stg1703\n");
		break;
	case ClockCh8398:
		Bprint(&stdout, "ch8398\n");
		break;
	case ClockInternal:
		Bprint(&stdout, "internal clock\n");
		Bprint(&stdout, "reference divider in plls\n");
		break;
	case ClockAtt20c408:
		Bprint(&stdout, "att 20c408\n");
		break;
	case ClockIbmrgb514:
		Bprint(&stdout, "ibm rgb514\n");
		Bprint(&stdout, "clock to program = 7\n");
		break;
	default:
		Bprint(&stdout, "unknown clock\n");
		break;
	}

	USED(mp);
	if(1 || mp->lcdpanelid) {
		lcdtable = *(ushort*)(bios+0x78);
		if(lcdtable+5 > sizeof bios || lcdtable+bios[lcdtable+5] > sizeof bios) {
			Bprint(&stdout, "can't find lcd bios table\n");
			goto NoLcd;
		}

		lcdpanel = *(ushort*)(bios+lcdtable+0x0A);
		if(lcdpanel+0x1D > sizeof bios /*|| bios[lcdpanel] != mp->lcdpanelid*/) {
			Bprint(&stdout, "can't find lcd bios table0\n");
			goto NoLcd;
		}

		Bprint(&stdout, "panelid %d x %d y %d\n", bios[lcdpanel], *(ushort*)(bios+lcdpanel+0x19), *(ushort*)(bios+lcdpanel+0x1B));
	}
NoLcd:;
}

Ctlr mach64xx = {
	"mach64xx",			/* name */
	snarf,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr mach64xxhwgc = {
	"mach64xxhwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};


