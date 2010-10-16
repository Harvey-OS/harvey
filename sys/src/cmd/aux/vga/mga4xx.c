/* Philippe Anel <philippe.anel@noos.fr> 
	- 2001-08-12 : First release.
	- 2001-08-15 : Added G450, with source code "adapted from" from Xfree86 4.1.0
	- 2001-08-23 : Added 'palettedepth 8' and a few 'ultradebug' ...
	- 2001-08-24 : Removed a possible lock in initialization.
	- 2001-08-30 : Hey ! The 32 bits mode is PALETIZED (Gamma Control I presume) !
				  And it seems plan9 assume the frame buffer is organized in
				  Big Endian format ! (+ Fix for the palette init. )
	- 2001-09-06 : Added Full 2D Accel ! (see drivers in /sys/src/9/pc)
	- 2001-10-01 : Rid Fix.
	- 2006-04-01 : Add MGA550 support.
	- 2006-08-07 : Add support for 16 and 24bits modes.
				HW accel now works for the G200 cards too (see kernel).
				by Leonardo Valencia <leoval@anixcorp.com>

     Greets and Acknowledgements go to :
     	- Sylvain Chipaux <a.k.a. asle>.
	- Nigel Roles.
	- Jean Mehat (the man who introduced me into the world of plan9).
	- Nicolas Stojanovic.
		... and for those who wrote plan9 of course ... :)
*/	

#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

static int	 ultradebug = 0;

/*
 * Matrox G4xx 3D graphics accelerators
 */
enum {
	Kilo			= 1024,
	Meg			= 1024*1024,
	
	MATROX			= 0x102B,	/* pci chip manufacturer */
	MGA550			= 0x2527, /* pci chip device ids */
	MGA4XX			= 0x0525,
	MGA200			= 0x0521,

	/* Pci configuration space mapping */
	PCfgMgaFBAA		= 0x10,		/* Frame buffer Aperture Address */
	PCfgMgaCAA		= 0x14,		/* Control Aperture Address base */
	PCfgMgaIAA		= 0x18,		/* ILOAD Aperture base Address */
	PCfgMgaOption1		= 0x40,		/* Option Register 1 */
	PCfgMgaOption2		= 0x50,		/* Option Register 2 */
	PCfgMgaOption3		= 0x54,		/* Option Register 3 */
	PCfgMgaDevCtrl		= 0x04,		/* Device Control */

	/* control aperture offsets */
	DMAWIN			= 0x0000,	/* 7KByte Pseudo-DMA Window */

	STATUS0			= 0x1FC2,	/* Input Status 0 */
	STATUS1			= 0x1FDA,	/* Input Status 1 */
	
	SEQIDX			= 0x1FC4,	/* Sequencer Index */
	SEQDATA			= 0x1FC5,	/* Sequencer Data */

	MISC_W			= 0x1FC2,	/* Misc. WO */
	MISC_R			= 0x1FCC,	/* Misc. RO */

	GCTLIDX			= 0x1FCE,	/* Graphic Controler Index */
	GCTLDATA		= 0x1FCF,	/* Graphic Controler Data */

	CRTCIDX			= 0x1FD4,	/* CRTC Index */
	CRTCDATA		= 0x1FD5,	/* CRTC Data */

	CRTCEXTIDX		= 0x1FDE,	/* CRTC Extension Index */
	CRTCEXTDATA		= 0x1FDF,	/* CRTC Extension Data */
	
	RAMDACIDX		= 0x3C00,	/* RAMDAC registers Index */
	RAMDACDATA		= 0x3C0A,	/* RAMDAC Indexed Data */
	RAMDACPALDATA		= 0x3C01,

	ATTRIDX			= 0x1FC0,	/* Attribute Index */
	ATTRDATA		= 0x1FC1,	/* Attribute Data */

	CACHEFLUSH		= 0x1FFF,

	C2_CTL			= 0X3C10,
	MGA_STATUS		= 0X1E14,
	Z_DEPTH_ORG		= 0X1C0C,
	
 	/* ... */
	Seq_ClockingMode =	0x01,
		Dotmode =		(1<<0),
		Shftldrt =		(1<<2),
		Dotclkrt =		(1<<3),
		Shiftfour =		(1<<4),
		Scroff =		(1<<5),

	CrtcExt_Horizontcount =	0x01,
		Htotal =		(1<<0),
		Hblkstr =		(1<<1),
		Hsyncstr =		(1<<2),
		Hrsten =		(1<<3),
		Hsyncoff =		(1<<4),
		Vsyncoff =		(1<<5),
		Hblkend =		(1<<6),
		Vrsten =		(1<<7),

	CrtcExt_Miscellaneous =	0x03,
		Mgamode =		(1<<7),

	Dac_Xpixclkctrl =	0x1a,
		Pixclksl = 		(3<<0),
		Pixclkdis =		(1<<2),
		Pixpllpdn =		(1<<3),

	Dac_Xpixpllstat =	0x4f,
		Pixlock = 		(1<<6),
	
	Dac_Xpixpllan =		0x45,
	Dac_Xpixpllbn =		0x49,
	Dac_Xpixpllcn  =	0x4d,

	Dac_Xpixpllam =		0x44, 
	Dac_Xpixpllbm =		0x48,
	Dac_Xpixpllcm =		0x4c,

	Dac_Xpixpllap =		0x46,
	Dac_Xpixpllbp =		0x4a,
	Dac_Xpixpllcp =		0x4e,

	Dac_Xmulctrl =		0x19,
		ColorDepth =		(7<<0),
			_8bitsPerPixel = 		0,
			_15bitsPerPixel =		1,
			_16bitsPerPixel =		2,
			_24bitsPerPixel =		3,
			_32bitsPerPixelWithOv = 	4,
			_32bitsPerPixel	=		7,

	Dac_Xpanelmode =	0x1f,

	Dac_Xmiscctrl =		0x1e,
		Dacpdn =		(1<<0),
		Mfcsel =		(3<<1),
		Vga8dac =		(1<<3),
		Ramcs =			(1<<4),
		Vdoutsel =		(7<<5),

	Dac_Xcurctrl =		0x06,
		CursorDis = 		0,
		Cursor3Color = 		1,
		CursorXGA = 		2,
		CursorX11 = 		3,
		Cursor16Color = 	4,

	Dac_Xzoomctrl =		0x38,

	Misc_loaddsel =			(1<<0),
	Misc_rammapen =			(1<<1),
	Misc_clksel =			(3<<2),
	Misc_videodis =			(1<<4),
	Misc_hpgoddev = 		(1<<5),
	Misc_hsyncpol =			(1<<6),
	Misc_vsyncpol =			(1<<7),

	MNP_TABLE_SIZE =	64,

	TRUE = 	(1 == 1),
	FALSE = (1 == 0),
};

typedef struct {
	Pcidev*	pci;
	int	devid;
	int	revid;
	
	uchar*	mmio;
	uchar*	mmfb;
	int	fbsize;
	ulong	iload;

	uchar	syspll_m;
	uchar	syspll_n;
	uchar	syspll_p;
	uchar	syspll_s;

	uchar	pixpll_m;
	uchar	pixpll_n;
	uchar	pixpll_p;
	uchar	pixpll_s;

	ulong	option1;
	ulong	option2;
	ulong	option3;

	ulong	Fneeded;

	/* From plan9.ini ... later */
	uchar	sdram;
	uchar	colorkey;
	uchar	maskkey;
	ulong	maxpclk;

	uchar	graphics[9];	
	uchar	attribute[0x14];	
	uchar	sequencer[5];
	uchar	crtc[0x19];
	uchar	crtcext[9];

	ulong	htotal;
	ulong	hdispend;
	ulong	hblkstr;
	ulong	hblkend;
	ulong	hsyncstr;
	ulong	hsyncend;
	ulong	vtotal;
	ulong	vdispend;
	ulong	vblkstr;
	ulong	vblkend;
	ulong	vsyncstr;
	ulong	vsyncend;
	ulong	linecomp;
	ulong	hsyncsel;
	ulong	startadd;
	ulong	offset;
	ulong	maxscan;
	ulong 	curloc;
	ulong	prowscan;
	ulong	currowstr;
	ulong	currowend;
	ulong	curoff;
	ulong	undrow;
	ulong	curskew;
	ulong	conv2t4;
	ulong	interlace;
	ulong	hsyncdel;
	ulong	hdispskew;
	ulong	bytepan;
	ulong	dotclkrt;
	ulong	dword;
	ulong	wbmode;
	ulong	addwrap;
	ulong	selrowscan;
	ulong	cms;
	ulong	csynccen;
	ulong	hrsten;
	ulong	vrsten;
	ulong	vinten;
	ulong	vintclr;
	ulong	hsyncoff;
	ulong	vsyncoff;
	ulong	crtcrstN;
	ulong	mgamode;
	ulong	scale;
	ulong	hiprilvl;
	ulong	maxhipri;
	ulong	c2hiprilvl;
	ulong	c2maxhipri;
	ulong	misc;
	ulong	crtcprotect;
	ulong	winsize;
	ulong	winfreq;
	
	ulong	mgaapsize;
} Mga;

static void
mgawrite32(Mga* mga, int index, ulong val)
{
	((ulong*)mga->mmio)[index] = val;
}

static ulong
mgaread32(Mga* mga, int index)
{
	return ((ulong*)mga->mmio)[index];
}

static void
mgawrite8(Mga* mga, int index, uchar val)
{
	mga->mmio[index] = val;
}

static uchar
mgaread8(Mga* mga, int index)
{
	return mga->mmio[index];
}

static uchar
seqget(Mga* mga, int index)
{
	mgawrite8(mga, SEQIDX, index);
	return mgaread8(mga, SEQDATA);
}

static uchar
seqset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, SEQIDX, index);
	tmp = mgaread8(mga, SEQDATA);
	mgawrite8(mga, SEQIDX, index);
	mgawrite8(mga, SEQDATA, (tmp & ~clr) | set);
	return tmp;
}

static uchar
crtcget(Mga* mga, int index)
{
	mgawrite8(mga, CRTCIDX, index);
	return mgaread8(mga, CRTCDATA);
}

static uchar
crtcset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, CRTCIDX, index);
	tmp = mgaread8(mga, CRTCDATA);
	mgawrite8(mga, CRTCIDX, index);
	mgawrite8(mga, CRTCDATA, (tmp & ~clr) | set);
	return tmp;
}

static uchar
crtcextget(Mga* mga, int index)
{
	mgawrite8(mga, CRTCEXTIDX, index);
	return mgaread8(mga, CRTCEXTDATA);
}

static uchar
crtcextset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, CRTCEXTIDX, index);
	tmp = mgaread8(mga, CRTCEXTDATA);
	mgawrite8(mga, CRTCEXTIDX, index);
	mgawrite8(mga, CRTCEXTDATA, (tmp & ~clr) | set);
	return tmp;
}

static uchar
dacget(Mga* mga, int index)
{
	mgawrite8(mga, RAMDACIDX, index);
	return mgaread8(mga, RAMDACDATA);
}

static uchar
dacset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, RAMDACIDX, index);
	tmp = mgaread8(mga, RAMDACDATA);
	mgawrite8(mga, RAMDACIDX, index);
	mgawrite8(mga, RAMDACDATA, (tmp & ~clr) | set);
	return	tmp;
}

static uchar
gctlget(Mga* mga, int index)
{
	mgawrite8(mga, GCTLIDX, index);
	return mgaread8(mga, GCTLDATA);
}

static uchar
gctlset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, GCTLIDX, index);
	tmp = mgaread8(mga, GCTLDATA);
	mgawrite8(mga, GCTLIDX, index);
	mgawrite8(mga, GCTLDATA, (tmp & ~clr) | set);
	return	tmp;
}

static uchar
attrget(Mga* mga, int index)
{
	mgawrite8(mga, ATTRIDX, index);
	return mgaread8(mga, ATTRDATA);
}

static uchar
attrset(Mga* mga, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(mga, ATTRIDX, index);
	tmp = mgaread8(mga, ATTRDATA);
	mgawrite8(mga, ATTRIDX, index);
	mgawrite8(mga, ATTRDATA, (tmp & ~clr) | set);
	return	tmp;
}

static uchar
miscget(Mga* mga)
{
	return mgaread8(mga, MISC_R);
}

static uchar
miscset(Mga* mga, uchar set, uchar clr)
{
	uchar	tmp;

	tmp = mgaread8(mga, MISC_R);
	mgawrite8(mga, MISC_W, (tmp & ~clr) | set);
	return	tmp;
}

/* ************************************************************ */

static void
dump_all_regs(Mga* mga)
{
	int	i;

	for (i = 0; i < 25; i++)
		trace("crtc[%d] = 0x%x\n", i, crtcget(mga, i));
	for (i = 0; i < 9; i++)
		trace("crtcext[%d] = 0x%x\n", i, crtcextget(mga, i));
	for (i = 0; i < 5; i++)
		trace("seq[%d] = 0x%x\n", i, seqget(mga, i));
	for (i = 0; i < 9; i++)
		trace("gctl[%d] = 0x%x\n", i, gctlget(mga, i));
	trace("misc = 0x%x\n", mgaread8(mga, MISC_R));
	for (i = 0; i < 0x87; i++)
		trace("dac[%d] = 0x%x\n", i, dacget(mga, i));
}

/* ************************************************************ */

static void
dump(Vga* vga, Ctlr* ctlr)
{
	dump_all_regs(vga->private);
	ctlr->flag |= Fdump;
}

static void
setpalettedepth(int depth)
{
	int	fd;
	char *cmd = strdup("palettedepth X");

	if ((depth != 8) && (depth != 6) && (depth != 16))
		error("mga: invalid palette depth %d\n", depth);

	fd = open("#v/vgactl", OWRITE);
	if(fd < 0)
		error("mga: can't open vgactl\n");

	cmd[13] = '0' + depth;
	if(write(fd, cmd, 14) != 14)
		error("mga: can't set palette depth to %d\n", depth);

	close(fd);
}

static void
mapmga4xx(Vga* vga, Ctlr* ctlr)
{
	int 	f;
	uchar* 	m;
	Mga *	mga;

	if(vga->private == nil)
		error("%s: g4xxio: no *mga4xx\n", ctlr->name);
	mga = vga->private;

	f = open("#v/vgactl", OWRITE);
	if(f < 0)
		error("%s: can't open vgactl\n", ctlr->name);

	if(write(f, "type mga4xx", 11) != 11)
		error("%s: can't set mga type\n", ctlr->name);
	
	m = segattach(0, "mga4xxmmio", 0, 16*Kilo);
	if(m == (void*)-1)
		error("%s: can't attach mga4xxmmio segment\n", ctlr->name);
	mga->mmio = m;
	trace("%s: mmio at %#p\n", ctlr->name, mga->mmio);

	m = segattach(0, "mga4xxscreen", 0, 32*Meg);
	if(m == (void*)-1) {
		mga->mgaapsize = 8*Meg;
		m = segattach(0, "mga4xxscreen", 0, 8*Meg);
		if(m == (void*)-1)
			error("%s: can't attach mga4xxscreen segment\n", ctlr->name);
	} else {
		mga->mgaapsize = 32*Meg;
	}
	mga->mmfb = m;
	trace("%s: frame buffer at %#p\n", ctlr->name, mga->mmfb);

	close(f);
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int 	i, k, n;
	uchar *	p;
	uchar	x[16];
	Pcidev *	pci;
	Mga *	mga;
	uchar	crtcext3;
	uchar	rid;

	trace("%s->snarf\n", ctlr->name);
	if(vga->private == nil) {
		pci = pcimatch(nil, MATROX, MGA4XX);
		if(pci == nil)
			pci = pcimatch(nil, MATROX, MGA550);
		if(pci == nil)
			pci = pcimatch(nil, MATROX, MGA200);
		if(pci == nil)
			error("%s: cannot find matrox adapter\n", ctlr->name);

		rid = pcicfgr8(pci, PciRID); // PciRID = 0x08

		trace("%s: G%d%d0 rev %d\n", ctlr->name, 
			2*(pci->did==MGA200)
			+4*(pci->did==MGA4XX)
			+5*(pci->did==MGA550),
			rid&0x80 ? 5 : 0,
			rid&~0x80);
		i = pcicfgr32(pci, PCfgMgaDevCtrl);
		if ((i & 2) != 2)
			error("%s: Memory Space not enabled ... Aborting ...\n", ctlr->name);	

		vga->private = alloc(sizeof(Mga));
		mga = (Mga*)vga->private;
		mga->devid = 	pci->did;
		mga->revid =	rid;	
		mga->pci = 	pci;

		mapmga4xx(vga, ctlr);
	}
	else {
		mga = (Mga*)vga->private;
	}

	/* Find out how much memory is here, some multiple of 2Meg */

	/* First Set MGA Mode ... */
	crtcext3 = crtcextset(mga, 3, 0x80, 0x00);

	p = mga->mmfb;
	n = (mga->mgaapsize / Meg) / 2;
	for (i = 0; i < n; i++) {
		k = (2*i+1)*Meg;
		p[k] = 0;
		p[k] = i+1;
		*(mga->mmio + CACHEFLUSH) = 0;
		x[i] = p[k];
		trace("x[%d]=%d\n", i, x[i]);
	}
	for(i = 1; i < n; i++)
		if(x[i] != i+1)
			break;
	vga->vmz = mga->fbsize = 2*i*Meg;
	trace("probe found %d megabytes\n", 2*i);

	crtcextset(mga, 3, crtcext3, 0xff);

	ctlr->flag |= Fsnarf;
}

static void
options(Vga* vga, Ctlr* ctlr)
{
	if(vga->virtx & 127)
		vga->virtx = (vga->virtx+127)&~127;
	ctlr->flag |= Foptions;
}

/* ************************************************************ */

static void 
G450ApplyPFactor(Mga*, uchar ucP, ulong *pulFIn)
{
	if(!(ucP & 0x40))
	{
		*pulFIn = *pulFIn / (2L << (ucP & 3));
	}
}


static void 
G450RemovePFactor(Mga*, uchar ucP, ulong *pulFIn)
{
	if(!(ucP & 0x40))
	{
		*pulFIn = *pulFIn * (2L << (ucP & 3));
	}
}

static void 
G450CalculVCO(Mga*, ulong ulMNP, ulong *pulF)
{
	uchar ucM, ucN;

	ucM = (uchar)((ulMNP >> 16) & 0xff);
	ucN = (uchar)((ulMNP >>  8) & 0xff);

	*pulF = (27000 * (2 * (ucN + 2)) + ((ucM + 1) >> 1)) / (ucM + 1);
	trace("G450CalculVCO: ulMNP %lx, pulF %ld\n", ulMNP, *pulF);
}


static void 
G450CalculDeltaFreq(Mga*, ulong ulF1, ulong ulF2, ulong *pulDelta)
{
	if(ulF2 < ulF1)
	{
		*pulDelta = ((ulF1 - ulF2) * 1000) / ulF1;
	}
	else
	{
		*pulDelta = ((ulF2 - ulF1) * 1000) / ulF1;
	}
	trace("G450CalculDeltaFreq: ulF1 %ld, ulF2 %ld, pulDelta %ld\n", ulF1, ulF2, *pulDelta);
}

static void 
G450FindNextPLLParam(Mga* mga, ulong ulFout, ulong *pulPLLMNP)
{
	uchar ucM, ucN, ucP, ucS;
	ulong ulVCO, ulVCOMin;

	ucM = (uchar)((*pulPLLMNP >> 16) & 0xff);
	/* ucN = (uchar)((*pulPLLMNP >>  8) & 0xff); */
	ucP = (uchar)(*pulPLLMNP &  0x43);

	ulVCOMin = 256000;

	if(ulVCOMin >= (255L * 8000))
	{
		ulVCOMin = 230000;
	}

	if((ucM == 9) && (ucP & 0x40))
	{
		*pulPLLMNP = 0xffffffff;
	} else if (ucM == 9)
	{
		if(ucP)
		{
			ucP--;
		}
		else
		{
			ucP = 0x40;
		}
		ucM = 0;
	}
	else
	{
		ucM++;
	}

	ulVCO = ulFout;

	G450RemovePFactor(mga, ucP, &ulVCO);

	if(ulVCO < ulVCOMin)
	{
		*pulPLLMNP = 0xffffffff;
	}

	if(*pulPLLMNP != 0xffffffff)
	{
		ucN = (uchar)(((ulVCO * (ucM+1) + 27000)/(27000 * 2)) - 2);

		ucS = 5;
		if(ulVCO < 1300000) ucS = 4;
		if(ulVCO < 1100000) ucS = 3;
		if(ulVCO <  900000) ucS = 2;
		if(ulVCO <  700000) ucS = 1;
		if(ulVCO <  550000) ucS = 0;

		ucP |= (uchar)(ucS << 3);

		*pulPLLMNP &= 0xff000000;
		*pulPLLMNP |= (ulong)ucM << 16;
		*pulPLLMNP |= (ulong)ucN << 8;
		*pulPLLMNP |= (ulong)ucP;
	}
}

static void 
G450FindFirstPLLParam(Mga* mga, ulong ulFout, ulong *pulPLLMNP)
{
	uchar ucP;
	ulong ulVCO;
	ulong ulVCOMax;

	/* Default value */
	ulVCOMax = 1300000;

	if(ulFout > (ulVCOMax/2))
	{
		ucP = 0x40;
		ulVCO = ulFout;
	}
	else
	{
		ucP = 3;
		ulVCO = ulFout;
		G450RemovePFactor(mga, ucP, &ulVCO);
		while(ucP && (ulVCO > ulVCOMax))
		{
			ucP--;
			ulVCO = ulFout;
			G450RemovePFactor(mga, ucP, &ulVCO);
		}
	}

	if(ulVCO > ulVCOMax)
	{
		*pulPLLMNP = 0xffffffff;
	}
	else
	{
		/* Pixel clock: 1 */
		*pulPLLMNP = (1 << 24) + 0xff0000 + ucP;
		G450FindNextPLLParam(mga, ulFout, pulPLLMNP);
	}
}


static void 
G450WriteMNP(Mga* mga, ulong ulMNP)
{
	if (0) trace("G450WriteMNP : 0x%lx\n", ulMNP);
	dacset(mga, Dac_Xpixpllcm, (uchar)(ulMNP >> 16), 0xff);
	dacset(mga, Dac_Xpixpllcn, (uchar)(ulMNP >>  8), 0xff);   
	dacset(mga, Dac_Xpixpllcp, (uchar)ulMNP, 0xff);   
}


static void 
G450CompareMNP(Mga* mga, ulong ulFout, ulong ulMNP1,
			    ulong ulMNP2, long *pulResult)
{
	ulong ulFreq, ulDelta1, ulDelta2;

	G450CalculVCO(mga, ulMNP1, &ulFreq);
	G450ApplyPFactor(mga, (uchar) ulMNP1, &ulFreq);
	G450CalculDeltaFreq(mga, ulFout, ulFreq, &ulDelta1);

	G450CalculVCO(mga, ulMNP2, &ulFreq);
	G450ApplyPFactor(mga, (uchar) ulMNP2, &ulFreq);
	G450CalculDeltaFreq(mga, ulFout, ulFreq, &ulDelta2);

	if(ulDelta1 < ulDelta2)
	{
		*pulResult = -1;
	}
	else if(ulDelta1 > ulDelta2)
	{
		*pulResult = 1;
	}
	else
	{
		*pulResult = 0;
	}

	if((ulDelta1 <= 5) && (ulDelta2 <= 5))
	{
		if((ulMNP1 & 0xff0000) < (ulMNP2 & 0xff0000))
		{
			*pulResult = -1;
		}
		else if((ulMNP1 & 0xff0000) > (ulMNP2 & 0xff0000))
		{
			*pulResult = 1;
		}
	}
}


static void 
G450IsPllLocked(Mga* mga, int *lpbLocked)
{
	ulong ulFallBackCounter, ulLockCount, ulCount;
	uchar  ucPLLStatus;

	/* Pixel PLL */
	mgawrite8(mga, 0x3c00, 0x4f);    
	ulFallBackCounter = 0;

	do 
	{
		ucPLLStatus = mgaread8(mga, 0x3c0a);
		if (0) trace("ucPLLStatus[1] : 0x%x\n", ucPLLStatus);
		ulFallBackCounter++;
	} while(!(ucPLLStatus & 0x40) && (ulFallBackCounter < 1000));

	ulLockCount = 0;
	if(ulFallBackCounter < 1000)
	{
		for(ulCount = 0; ulCount < 100; ulCount++)
		{
			ucPLLStatus = mgaread8(mga, 0x3c0a);
			if (0) trace("ucPLLStatus[2] : 0x%x\n", ucPLLStatus);
			if(ucPLLStatus & 0x40)
			{
				ulLockCount++;
			}
		}
	}

	*lpbLocked = ulLockCount >= 90;
}

static void 
G450SetPLLFreq(Mga* mga, long f_out) 
{
	int bFoundValidPLL;
	int bLocked;
	ulong ulMaxIndex;
	ulong ulMNP;
	ulong ulMNPTable[MNP_TABLE_SIZE];
	ulong ulIndex;
	ulong ulTryMNP;
	long lCompareResult;

	trace("f_out : %ld\n", f_out);

	G450FindFirstPLLParam(mga, f_out, &ulMNP);
	ulMNPTable[0] = ulMNP;
	G450FindNextPLLParam(mga, f_out, &ulMNP);
	ulMaxIndex = 1;
	while(ulMNP != 0xffffffff)
	{
		int ulIndex;
		int bSkipValue;

		bSkipValue = FALSE;
		if(ulMaxIndex == MNP_TABLE_SIZE)
		{
			G450CompareMNP(mga, f_out, ulMNP, ulMNPTable[MNP_TABLE_SIZE - 1],
				       &lCompareResult);

			if(lCompareResult > 0)
			{
				bSkipValue = TRUE;
			}
			else
			{
				ulMaxIndex--;
			}
		}

		if(!bSkipValue)
		{
			for(ulIndex = ulMaxIndex; !bSkipValue && (ulIndex > 0); ulIndex--)
			{
				G450CompareMNP(mga, f_out, ulMNP, ulMNPTable[ulIndex - 1],
					       &lCompareResult);

				if(lCompareResult < 0)
				{
					ulMNPTable[ulIndex] = ulMNPTable[ulIndex - 1];
				}
				else
				{
					break;
				}
			}
			ulMNPTable[ulIndex] = ulMNP;
			ulMaxIndex++;
		}

		G450FindNextPLLParam(mga, f_out, &ulMNP);
	}

	bFoundValidPLL = FALSE;
	ulMNP = 0;

	for(ulIndex = 0; !bFoundValidPLL && (ulIndex < ulMaxIndex); ulIndex++)
	{
		ulTryMNP = ulMNPTable[ulIndex];

		{
			bLocked = TRUE;
			if((ulMNPTable[ulIndex] & 0xff00) < 0x300 ||
			   (ulMNPTable[ulIndex] & 0xff00) > 0x7a00)
			{
				bLocked = FALSE;
			}

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP - 0x300);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP + 0x300);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP - 0x200);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP + 0x200);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP - 0x100);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP + 0x100);
				G450IsPllLocked(mga, &bLocked);
			}     

			if(bLocked)
			{
				G450WriteMNP(mga, ulTryMNP);
				G450IsPllLocked(mga, &bLocked);
			}     
			else if(!ulMNP)
			{
				G450WriteMNP(mga, ulTryMNP);
				G450IsPllLocked(mga, &bLocked);
				if(bLocked)
				{
					ulMNP = ulMNPTable[ulIndex]; 
				}
				bLocked = FALSE;
			}

			if(bLocked)
			{
				bFoundValidPLL = TRUE;
			}
		}
	}

	if(!bFoundValidPLL)
	{
		if(ulMNP)
		{
			G450WriteMNP(mga, ulMNP);
		}
		else
		{
			G450WriteMNP(mga, ulMNPTable[0]);
		}
	}
}


/* ************************************************************ */

/*
	calcclock - Calculate the PLL settings (m, n, p, s).
*/
static double
g400_calcclock(Mga* mga, long Fneeded)
{
	double	Fpll;
	double	Fvco;
	double 	Fref;
	int		pixpll_m_min;
	int		pixpll_m_max;
	int		pixpll_n_min;
	int		pixpll_n_max;
	int		pixpll_p_max;
	double 	Ferr, Fcalc;
	int		m, n, p;
	
	if (mga->devid == MGA4XX || mga->devid == MGA550) {
		/* These values are taken from Matrox G400 Specification - p 4-91 */
		Fref     		= 27000000.0;
		pixpll_n_min 	= 7;
		pixpll_n_max 	= 127;
		pixpll_m_min	= 1;
		pixpll_m_max	= 31;
		pixpll_p_max 	= 7;
	} else { 				/* MGA200 */
		/* These values are taken from Matrox G200 Specification - p 4-77 */
		//Fref     		= 14318180.0;
		Fref     		= 27050500.0;
		pixpll_n_min 	= 7;
		pixpll_n_max 	= 127;
		pixpll_m_min	= 1;
		pixpll_m_max	= 6;
		pixpll_p_max 	= 7;
	}

	Fvco = ( double ) Fneeded;
	for (p = 0;  p <= pixpll_p_max && Fvco < mga->maxpclk; p = p * 2 + 1, Fvco *= 2.0)
		;
	mga->pixpll_p = p;

	Ferr = Fneeded;
	for ( m = pixpll_m_min ; m <= pixpll_m_max ; m++ )
		for ( n = pixpll_n_min; n <= pixpll_n_max; n++ )
		{ 
			Fcalc = Fref * (n + 1) / (m + 1) ;

			/*
			 * Pick the closest frequency.
			 */
			if ( labs(Fcalc - Fvco) < Ferr ) {
				Ferr = abs(Fcalc - Fvco);
				mga->pixpll_m = m;
				mga->pixpll_n = n;
			}
		}
	
	Fvco = Fref * (mga->pixpll_n + 1) / (mga->pixpll_m + 1);

	if (mga->devid == MGA4XX || mga->devid == MGA550) {
		if ( (50000000.0 <= Fvco) && (Fvco < 110000000.0) )
			mga->pixpll_p |= 0;	
		if ( (110000000.0 <= Fvco) && (Fvco < 170000000.0) )
			mga->pixpll_p |= (1<<3);	
		if ( (170000000.0 <= Fvco) && (Fvco < 240000000.0) )
			mga->pixpll_p |= (2<<3);	
		if ( (300000000.0 <= Fvco) )
			mga->pixpll_p |= (3<<3);	
	} else {
		if ( (50000000.0 <= Fvco) && (Fvco < 100000000.0) )
			mga->pixpll_p |= 0;	
		if ( (100000000.0 <= Fvco) && (Fvco < 140000000.0) )
			mga->pixpll_p |= (1<<3);	
		if ( (140000000.0 <= Fvco) && (Fvco < 180000000.0) )
			mga->pixpll_p |= (2<<3);	
		if ( (250000000.0 <= Fvco) )
			mga->pixpll_p |= (3<<3);	
	}
	
	Fpll = Fvco / (p + 1);

	return Fpll;
}

/* ************************************************************ */

static void
init(Vga* vga, Ctlr* ctlr)
{
	Mode*	mode;
	Mga*	mga;
	double	Fpll;
	Ctlr*	c;
	int	i;
	ulong	t;
	int     bppShift;

	mga = vga->private;
	mode = vga->mode;

	trace("mga mmio at %#p\n", mga->mmio);

	ctlr->flag |= Ulinear;

	/*
	 * Set the right bppShitf based on depth
	 */

	switch(mode->z) {
	case 8: 
		bppShift = 0;
		break;
	case 16:
		bppShift = 1;
		break;
	case 24:
		bppShift = 0;
		break;
	case 32:
		bppShift = 2;
		break;
	default:
		bppShift = 0;
		error("depth %d not supported !\n", mode->z);
	}

	if (mode->interlace)
		error("interlaced mode not supported !\n");

	trace("%s: Initializing mode %dx%dx%d on %s\n", ctlr->name, mode->x, mode->y, mode->z, mode->type);
	trace("%s: Suggested Dot Clock : %d\n", 	ctlr->name, mode->frequency);
	trace("%s: Horizontal Total = %d\n", 		ctlr->name, mode->ht);
	trace("%s: Start Horizontal Blank = %d\n", ctlr->name, mode->shb);
	trace("%s: End Horizontal Blank = %d\n", 	ctlr->name, mode->ehb);
	trace("%s: Vertical Total = %d\n", 		ctlr->name, mode->vt);
	trace("%s: Vertical Retrace Start = %d\n", 	ctlr->name, mode->vrs);
	trace("%s: Vertical Retrace End = %d\n", 	ctlr->name, mode->vre);
	trace("%s: Start Horizontal Sync = %d\n", 	ctlr->name, mode->shs);
	trace("%s: End Horizontal Sync = %d\n", 	ctlr->name, mode->ehs);
	trace("%s: HSync = %c\n", 			ctlr->name, mode->hsync);
	trace("%s: VSync = %c\n", 				ctlr->name, mode->vsync);
	trace("%s: Interlace = %d\n", 			ctlr->name, mode->interlace);

	/* TODO : G400 Max : 360000000 */
	if (mga->devid == MGA4XX || mga->devid == MGA550)
		mga->maxpclk	= 300000000;
	else
		mga->maxpclk	= 250000000;

	if (mode->frequency < 50000)
		error("mga: Too little Frequency %d : Minimum supported by PLL is %d\n", 
			mode->frequency, 50000);

	if (mode->frequency > mga->maxpclk)
		error("mga: Too big Frequency %d : Maximum supported by PLL is %ld\n",
			mode->frequency, mga->maxpclk);
	
	trace("mga: revision ID is %x\n", mga->revid);
	if ((mga->devid == MGA200) || ((mga->devid == MGA4XX) && (mga->revid & 0x80) == 0x00)) {
		/* Is it G200/G400 or G450 ? */
		Fpll = g400_calcclock(mga, mode->frequency);
		trace("Fpll set to %f\n", Fpll);
		trace("pixclks : n = %d m = %d p = %d\n", mga->pixpll_n, mga->pixpll_m, mga->pixpll_p & 0x7);
	} else
		mga->Fneeded = mode->frequency;

	trace("PCI Option1 = 0x%x\n", pcicfgr32(mga->pci, PCfgMgaOption1));
	trace("PCI Option2 = 0x%x\n", pcicfgr32(mga->pci, PCfgMgaOption2));
	trace("PCI Option3 = 0x%x\n", pcicfgr32(mga->pci, PCfgMgaOption3));

	mga->htotal =			(mode->ht >> 3) - 5;
	mga->hdispend =		(mode->x >> 3) - 1;
	mga->hblkstr =		mga->hdispend; 		/* (mode->shb >> 3); */
	mga->hblkend =		mga->htotal + 4;		/* (mode->ehb >> 3); */
	mga->hsyncstr =		(mode->shs >> 3) - 1;	// Was (mode->shs >> 3);
	mga->hsyncend =		(mode->ehs >> 3) - 1;	// Was (mode->ehs >> 3);
	mga->hsyncdel = 		0;
	mga->vtotal =			mode->vt - 2;
	mga->vdispend = 		mode->y - 1;
	mga->vblkstr = 		mode->y - 1;
	mga->vblkend = 		mode->vt - 1;
	mga->vsyncstr = 		mode->vrs;
	mga->vsyncend = 		mode->vre;
	mga->linecomp =		mode->y;
	mga->hsyncsel = 		0;					/* Do not double lines ... */
	mga->startadd =		0;
	mga->offset =		(mode->z==24) ? (vga->virtx * 3) >> (4 - bppShift) : vga->virtx >> (4-bppShift);
	/* No Zoom */
	mga->maxscan = 		0;
	/* Not used in Power Graphic mode */
	mga->curloc =			0;
	mga->prowscan = 		0;
	mga->currowstr = 		0;
	mga->currowend = 		0;
	mga->curoff = 		1;
	mga->undrow = 		0;
	mga->curskew = 		0;
	mga->conv2t4 = 		0;
	mga->interlace =		0;
	mga->hdispskew =		0;
	mga->bytepan = 		0;
	mga->dotclkrt = 		0;
	mga->dword =			0;
	mga->wbmode =		1;
	mga->addwrap = 		0;	/* Not Used ! */
	mga->selrowscan =		1;
	mga->cms =			1;
	mga->csynccen =		0; 	/* Disable composite sync */

	/* VIDRST Pin */
	mga->hrsten =			0; 	// Was 1;
	mga->vrsten =			0; 	// Was 1;

	/* vertical interrupt control ... disabled */
	mga->vinten = 		1;
	mga->vintclr = 		0;

	/* Let [hv]sync run freely */
	mga->hsyncoff =		0;
	mga->vsyncoff =		0;

	mga->crtcrstN =		1;

	mga->mgamode = 		1;
	mga->scale   =		(mode->z == 24) ? ((1 << bppShift)*3)-1 : (1 << bppShift)-1;
	
	mga->crtcprotect =      1;
	mga->winsize = 		0;
	mga->winfreq = 		0;

	if ((mga->htotal == 0)
	    || (mga->hblkend <= (mga->hblkstr + 1))
	    || ((mga->htotal - mga->hdispend) == 0)
	    || ((mga->htotal - mga->bytepan + 2) <= mga->hdispend)
	    || (mga->hsyncstr <= (mga->hdispend + 2))
	    || (mga->vtotal == 0))
	{
		error("Invalid Power Graphic Mode :\n"
		      "mga->htotal = %ld\n"
		      "mga->hdispend = %ld\n"
		      "mga->hblkstr = %ld\n"
		      "mga->hblkend = %ld\n"
		      "mga->hsyncstr = %ld\n"
		      "mga->hsyncend = %ld\n"
		      "mga->hsyncdel = %ld\n"
		      "mga->vtotal = %ld\n"
		      "mga->vdispend = %ld\n"
		      "mga->vblkstr = %ld\n"
		      "mga->vblkend = %ld\n"
		      "mga->vsyncstr = %ld\n"
		      "mga->vsyncend = %ld\n"
		      "mga->linecomp = %ld\n",
		      mga->htotal,
		      mga->hdispend,
		      mga->hblkstr,
		      mga->hblkend,
		      mga->hsyncstr,
		      mga->hsyncend,
		      mga->hsyncdel,
		      mga->vtotal,
		      mga->vdispend,
		      mga->vblkstr,
		      mga->vblkend,
		      mga->vsyncstr,
		      mga->vsyncend,
		      mga->linecomp
		      );
	}

	mga->hiprilvl = 0;
	mga->maxhipri = 0;
	mga->c2hiprilvl = 0;
	mga->c2maxhipri = 0;

	mga->misc = ((mode->hsync != '-')?0:(1<<6)) | ((mode->vsync != '-')?0:(1<<7));

	trace("mga->htotal = %ld\n"
		      "mga->hdispend = %ld\n"
		      "mga->hblkstr = %ld\n"
		      "mga->hblkend = %ld\n"
		      "mga->hsyncstr = %ld\n"
		      "mga->hsyncend = %ld\n"
		      "mga->hsyncdel = %ld\n"
		      "mga->vtotal = %ld\n"
		      "mga->vdispend = %ld\n"
		      "mga->vblkstr = %ld\n"
		      "mga->vblkend = %ld\n"
		      "mga->vsyncstr = %ld\n"
		      "mga->vsyncend = %ld\n"
		      "mga->linecomp = %ld\n",
		      mga->htotal,
		      mga->hdispend,
		      mga->hblkstr,
		      mga->hblkend,
		      mga->hsyncstr,
		      mga->hsyncend,
		      mga->hsyncdel,
		      mga->vtotal,
		      mga->vdispend,
		      mga->vblkstr,
		      mga->vblkend,
		      mga->vsyncstr,
		      mga->vsyncend,
		      mga->linecomp
		      );	

	mga->crtc[0x00] = 0xff & mga->htotal;

	mga->crtc[0x01] = 0xff & mga->hdispend;

	mga->crtc[0x02] = 0xff & mga->hblkstr;

	mga->crtc[0x03] = (0x1f & mga->hblkend) 
		| ((0x03 & mga->hdispskew) << 5)
		| 0x80	/* cf 3-304 */
		;

	mga->crtc[0x04] = 0xff & mga->hsyncstr;

	mga->crtc[0x05] = (0x1f & mga->hsyncend) 
		| ((0x03 & mga->hsyncdel) << 5) 
		| ((0x01 & (mga->hblkend >> 5)) << 7)
		;

	mga->crtc[0x06] = 0xff & mga->vtotal;

	t = ((0x01 & (mga->vtotal >> 8)) << 0)
	  | ((0x01 & (mga->vdispend >> 8)) << 1)
	  | ((0x01 & (mga->vsyncstr >> 8)) << 2)
	  | ((0x01 & (mga->vblkstr >> 8)) << 3)
	  | ((0x01 & (mga->linecomp >> 8)) << 4)
	  | ((0x01 & (mga->vtotal >> 9)) << 5)
	  | ((0x01 & (mga->vdispend >> 9)) << 6)
	  | ((0x01 & (mga->vsyncstr >> 9)) << 7)
	  ;
	mga->crtc[0x07] = 0xff & t;

	mga->crtc[0x08] = (0x1f & mga->prowscan) 
		| ((0x03 & mga->bytepan) << 5)
		;

	mga->crtc[0x09] = (0x1f & mga->maxscan) 
		| ((0x01 & (mga->vblkstr >> 9)) << 5)
		| ((0x01 & (mga->linecomp >> 9)) << 6)
		| ((0x01 & mga->conv2t4) << 7)
		;

	mga->crtc[0x0a] = (0x1f & mga->currowstr)
		| ((0x01 & mga->curoff) << 5)
		;

	mga->crtc[0x0b] = (0x1f & mga->currowend)
		| ((0x03 & mga->curskew) << 5)
		;

	mga->crtc[0x0c] = 0xff & (mga->startadd >> 8);

	mga->crtc[0x0d] = 0xff & mga->startadd;

	mga->crtc[0x0e] = 0xff & (mga->curloc >> 8);

	mga->crtc[0x0f] = 0xff & mga->curloc;

	mga->crtc[0x10] = 0xff & mga->vsyncstr;

	mga->crtc[0x11] = (0x0f & mga->vsyncend)
		| ((0x01 & mga->vintclr) << 4)
		| ((0x01 & mga->vinten) << 5)
		| ((0x01 & mga->crtcprotect) << 7)
		;

	mga->crtc[0x12] = 0xff & mga->vdispend;

	mga->crtc[0x13] = 0xff & mga->offset;

	mga->crtc[0x14] = 0x1f & mga->undrow;	/* vga only */

	mga->crtc[0x15] = 0xff & mga->vblkstr;

	mga->crtc[0x16] = 0xff & mga->vblkend;

	mga->crtc[0x17] = ((0x01 & mga->cms) << 0)
		| ((0x01 & mga->selrowscan) << 1)
		| ((0x01 & mga->hsyncsel) << 2)
		| ((0x01 & mga->addwrap) << 5)
		| ((0x01 & mga->wbmode) << 6)
		| ((0x01 & mga->crtcrstN) << 7)
		;

	mga->crtc[0x18] = 0xff & mga->linecomp;
	
	mga->crtcext[0] = (0x0f & (mga->startadd >> 16))
		| ((0x03 & (mga->offset >> 8)) << 4)
		| ((0x01 & (mga->startadd >> 20)) << 6)
		| ((0x01 & mga->interlace) << 7)
		;

	mga->crtcext[1] = ((0x01 & (mga->htotal >> 8)) << 0)
		| ((0x01 & (mga->hblkstr >> 8)) << 1)
		| ((0x01 & (mga->hsyncstr >> 8)) << 2)
		| ((0x01 & mga->hrsten) << 3)
		| ((0x01 & mga->hsyncoff) << 4)
		| ((0x01 & mga->vsyncoff) << 5)
		| ((0x01 & (mga->hblkend >> 6)) << 6)
		| ((0x01 & mga->vrsten) << 7)
		;

	mga->crtcext[2] = ((0x03 & (mga->vtotal >> 10)) << 0)
		| ((0x01 & (mga->vdispend >> 10)) << 2)
		| ((0x03 & (mga->vblkstr >> 10)) << 3)
		| ((0x03 & (mga->vsyncstr >> 10)) << 5)
		| ((0x01 & (mga->linecomp >> 10)) << 7)
		;

	mga->crtcext[3] = ((0x07 & mga->scale) << 0)
		| ((0x01 & mga->csynccen) << 6)
		| ((0x01 & mga->mgamode) << 7)
		;

	mga->crtcext[4] = 0;	/* memory page ... not used in Power Graphic Mode */

	mga->crtcext[5] = 0;	/* Not used in non-interlaced mode */

	mga->crtcext[6] = ((0x07 & mga->hiprilvl) << 0)
		| ((0x07 & mga->maxhipri) << 4)
		;

	mga->crtcext[7] = ((0x07 & mga->winsize) << 1)
		| ((0x07 & mga->winfreq) << 5)
		;

	mga->crtcext[8] = (0x01 & (mga->startadd >> 21)) << 0;

	/* Initialize Sequencer */
	mga->sequencer[0] = 0;
	mga->sequencer[1] = 0;
	mga->sequencer[2] = 0x03;
	mga->sequencer[3] = 0;
	mga->sequencer[4] = 0x02;

	/* Graphic Control registers are ignored when not using 0xA0000 aperture */	
	for (i = 0; i < 9; i++)
		mga->graphics[i] = 0;	

	/* The Attribute Controler is not available in Power Graphics mode */
	for (i = 0; i < 0x15; i++)
		mga->attribute[i] = i;	

	/* disable vga load (want to do fields in different order) */
	for(c = vga->link; c; c = c->link)
		if (strncmp(c->name, "vga", 3) == 0)
			c->load = nil;
}

static void
load(Vga* vga, Ctlr* ctlr)
{
	Mga*	mga;
	int	i;
	uchar*	p;
	Mode*	mode;
	uchar	cursor;

	mga = vga->private;
	mode = vga->mode;

	trace("mga: Loading ...\n");
	dump_all_regs(mga);

	if (mode->z == 8)
		setpalettedepth(mode->z);

	trace("mga mmio at %#p\n", mga->mmio);
	trace("mga: loading vga registers ...\n" );
	if (ultradebug) Bflush(&stdout);

	/* Initialize Sequencer registers */
	for(i = 0; i < 5; i++)
		seqset(mga, i, mga->sequencer[i], 0xff);

	/* Initialize Attribute register */
	for(i = 0; i < 0x15; i++)
		attrset(mga, i, mga->attribute[i], 0xff);

	/* Initialize Graphic Control registers */
	for(i = 0; i < 9; i++)
		gctlset(mga, i, mga->graphics[i], 0xff);

	/* Wait VSYNC */
	while (mgaread8(mga, STATUS1) & 0x08);
	while (! (mgaread8(mga, STATUS1) & ~0x08));

	/* Turn off the video. */
	seqset(mga, Seq_ClockingMode, Scroff, 0);

	/* Crtc2 Off */
	mgawrite32(mga, C2_CTL, 0);

	/* Disable Cursor */
	cursor = dacset(mga, Dac_Xcurctrl, CursorDis, 0xff);

	/* Pixel Pll UP and set Pixel clock source to Pixel Clock PLL */
	dacset(mga, Dac_Xpixclkctrl, 0x01 | 0x08, 0x0f);

	trace("mga: waiting for the clock source becomes stable ...\n");
	while ((dacget(mga, Dac_Xpixpllstat) & Pixlock) != Pixlock)
		;
	trace("mga: pixpll locked !\n");
	if (ultradebug) Bflush(&stdout);

	/* Enable LUT, Disable MAFC */
	dacset(mga, Dac_Xmiscctrl, Ramcs | Mfcsel, Vdoutsel);

	/* Disable Dac */
	dacset(mga, Dac_Xmiscctrl, 0, Dacpdn);

	/* Initialize Panel Mode */
	dacset(mga, Dac_Xpanelmode, 0, 0xff);

	/* Disable the PIXCLK and set Pixel clock source to Pixel Clock PLL */
	dacset(mga, Dac_Xpixclkctrl, Pixclkdis | 0x01, 0x3);

	/* Disable mapping of the memory */ 
	miscset(mga, 0, Misc_rammapen);

	/* Enable 8 bit palette */
	dacset(mga, Dac_Xmiscctrl, Vga8dac, 0);

	/* Select MGA Pixel Clock */
	miscset(mga, Misc_clksel, 0);

	/* Initialize Z Buffer ... (useful?) */
	mgawrite32(mga, Z_DEPTH_ORG, 0);

	/* Wait */
	for (i = 0; i < 50; i++)
		mgaread32(mga, MGA_STATUS);

	if ((mga->devid == MGA200) || ((mga->devid == MGA4XX) && (mga->revid & 0x80) == 0x00)) { 
		dacset(mga, Dac_Xpixpllcm, mga->pixpll_m, 0xff);
		dacset(mga, Dac_Xpixpllcn, mga->pixpll_n, 0xff);
		dacset(mga, Dac_Xpixpllcp, mga->pixpll_p, 0xff);

		/* Wait until new clock becomes stable */
		trace("mga: waiting for the clock source becomes stable ...\n");
		while ((dacget(mga, Dac_Xpixpllstat) & Pixlock) != Pixlock)
			;
		trace("mga: pixpll locked !\n");
	} else {
	/* MGA450 and MGA550 */
		/* Wait until new clock becomes stable */
		trace("mga450: waiting for the clock source becomes stable ...\n");
		while ((dacget(mga, Dac_Xpixpllstat) & Pixlock) != Pixlock)
			;
		trace("mga: pixpll locked !\n");

		G450SetPLLFreq(mga, (long) mga->Fneeded / 1000);
	}

	/* Enable Pixel Clock Oscillation */
	dacset(mga, Dac_Xpixclkctrl, 0, Pixclkdis);
	if (ultradebug) Bflush(&stdout);

	/* Enable Dac */
	dacset(mga, Dac_Xmiscctrl, Dacpdn, 0);

	/* Set Video Mode */
	switch (mode->z) {
	case 8:
		dacset(mga, Dac_Xmulctrl, _8bitsPerPixel, ColorDepth);	
		break;
	case 16:
		dacset(mga, Dac_Xmulctrl, _16bitsPerPixel, ColorDepth);	
		break;
	case 24:
		dacset(mga, Dac_Xmulctrl, _24bitsPerPixel, ColorDepth);	
		break;
	case 32:
		dacset(mga, Dac_Xmulctrl, _32bitsPerPixel, ColorDepth);
		break;
	default: 
		error("Unsupported depth %d\n", mode->z);
	}

	/* Wait */
	for (i = 0; i < 50; i++)
		mgaread32(mga, MGA_STATUS);

	/* Wait until new clock becomes stable */
	trace("mga: waiting for the clock source becomes stable ...\n");
	if (ultradebug) Bflush(&stdout);
	while ((dacget(mga, Dac_Xpixpllstat) & Pixlock) != Pixlock)
		;
	trace("mga: pixpll locked !\n");
	if (ultradebug) Bflush(&stdout);

	/* Initialize CRTC registers and remove irq */
	crtcset(mga, 0x11, (1<<4), (1<<5)|0x80);
	for (i = 0; i < 25; i++)
		crtcset(mga, i, mga->crtc[i], 0xff);

	trace("mga: crtc loaded !\n");
	if (ultradebug) Bflush(&stdout);

	/* Initialize CRTC Extension registers */
	for (i = 0; i < 9; i++)
		crtcextset(mga, i, mga->crtcext[i], 0xff);

	trace("mga: ext loaded !\n");
	if (ultradebug) Bflush(&stdout);

	/* Disable Zoom */
	dacset(mga, Dac_Xzoomctrl, 0, 0xff);

	trace("mga: XzoomCtrl Loaded !\n");
	if (ultradebug) Bflush(&stdout);

	/* Enable mga mode again ... Just in case :) */
	crtcextset(mga, CrtcExt_Miscellaneous, Mgamode, 0);

	trace("mga: crtcext MgaMode loaded !\n");
	if (ultradebug) Bflush(&stdout);

	if (mode->z == 32 || mode->z == 24 ) {
		/* Initialize Big Endian Mode ! */
		mgawrite32(mga, 0x1e54, 0x02 << 16);
	}


	/* Set final misc ... enable mapping ... */
	miscset(mga, mga->misc | Misc_rammapen, 0);

	trace("mga: mapping enabled !\n");
	if (ultradebug) Bflush(&stdout);

	/* Enable Screen */
	seqset(mga, 1, 0, 0xff);

	trace("screen enabled ...\n");

	if (0) {
		p = mga->mmfb;
		for (i = 0; i < mga->fbsize; i++)
			*p++ = (0xff & i);
	}

	trace("mga: Loaded !\n" );
	dump_all_regs(mga);
	if (ultradebug) Bflush(&stdout);

	trace("mga: Loaded [bis]!\n" );

	/*
	 * TODO: In 16bpp mode, what is the correct palette ?
	 *       in the meantime lets use the default one,
	 *       which has a weird color combination.
	 */

	if (mode->z != 8 && mode ->z != 16) {
		/* Initialize Palette */
		mgawrite8(mga, RAMDACIDX, 0);
		for (i = 0; i < 0x100; i++) {
			mgawrite8(mga, RAMDACPALDATA, i);
			mgawrite8(mga, RAMDACPALDATA, i);
			mgawrite8(mga, RAMDACPALDATA, i);
		}
	}

	trace("mga: Palette initialised !\n");

	/* Enable Cursor */
	dacset(mga, Dac_Xcurctrl, cursor, 0xff);

	ctlr->flag |= Fload;
	if (ultradebug) Bflush(&stdout);
}

Ctlr mga4xx = {
	"mga4xx",			/* name */
	snarf,				/* snarf */
	options,				/* options */
	init,					/* init */
	load,					/* load */
	dump,				/* dump */
};

Ctlr mga4xxhwgc = {
	"mga4xxhwgc",		/* name */
	0,					/* snarf */
	0,					/* options */
	0,					/* init */
	0,					/* load */
	dump,				/* dump */
};
