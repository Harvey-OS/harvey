#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

enum {
	PCIS3		= 0x5333,		/* PCI VID */

	SAVAGE3D	= 0x8A20,	/* PCI DID */
	SAVAGE3DMV	= 0x8A21,
	SAVAGE4		= 0x8A22,
	PROSAVAGEP	= 0x8A25,
	PROSAVAGEK	= 0x8A26,
	PROSAVAGE8	= 0x8D04,
	SAVAGEMXMV	= 0x8C10,
	SAVAGEMX	= 0x8C11,
	SAVAGEIXMV	= 0x8C12,
	SAVAGEIX	= 0x8C13,
	SUPERSAVAGEIXC16 = 0x8C2E,
	SAVAGE2000	= 0x9102,

	VIRGE		= 0x5631,
	VIRGEGX2	= 0x8A10,
	VIRGEDXGX	= 0x8A01,
	VIRGEVX		= 0x883D,
	VIRGEMX		= 0x8C01,
	VIRGEMXP	= 0x8C03,

	AURORA64VPLUS	= 0x8812,
};

/*
 * Savage4 et al. acceleration.
 *
 * This is based only on the Savage4 documentation.
 * It is expected to work on other Savage cards as well,
 * but has not been tried.
 * 
 * There are five ways to access the 2D graphics engine registers:
 * 	- Old MMIO non-packed format
 *	- Old MMIO packed format
 *	- New MMIO non-packed format
 *	- New MMIO packed format
 *	- Burst Command Interface (BCI)
 *
 * Of these, the manual hints that the first three are deprecated,
 * and it does not document any of those three well enough to use.
 * 
 * I have tried for many hours with no success to understand the BCI
 * interface well enough to use it.  It is not well documented, and the
 * XFree86 driver seems to completely contradict what little documentation
 * there is.
 *
 * This leaves the packed new MMIO.
 * The manual contradicts itself here, claming that the registers
 * start at 0x2008100 as well as at 0x0008100 from the base of the 
 * mmio segment.  Since the segment is only 512k, we assume that
 * the latter is the correct offset.
 *
 * According to the manual, only 16-bit reads of the 2D registers
 * are supported: 32-bit reads will return garbage in the upper word.
 * 32-bit writes must be enabled explicitly.
 * 
 * 32-bit reads of the status registers seem just fine.
 */

/* 2D graphics engine registers for Savage4; others appear to be mostly the same */
enum {
	SubsystemStatus = 0x8504,	/* Subsystem Status: read only */
	  /* read only: whether we get interrupts on various events */
	  VsyncInt		= 1<<0,		/* vertical sync */
	  GeBusyInt		= 1<<1,		/* 2D graphics engine busy */
	  BfifoFullInt	= 1<<2,		/* BIU FIFO full */
	  BfifoEmptyInt	= 1<<3,		/* BIU FIFO empty */
	  CfifoFullInt	= 1<<4,		/* command FIFO full */
	  CfifoEmptyInt	= 1<<5,		/* command FIFO empty */
	  BciInt		= 1<<6, 	/* BCI */
	  LpbInt		= 1<<7, 	/* LPB */
	  CbHiInt		= 1<<16,	/* COB upper threshold */
	  CbLoInt		= 1<<17,	/* COB lower threshold */

	SubsystemCtl 	= 0x8504,	/* Subsystem Control: write only */
	  /* clear interrupts for various events */
	  VsyncClr		= 1<<0,
	  GeBusyClr		= 1<<1,
	  BfifoFullClr	= 1<<2,
	  BfifoEmptyClr	= 1<<3,
	  CfifoFullClr	= 1<<4,
	  CfifoEmptyClr	= 1<<5,
	  BciClr		= 1<<6,
	  LpbClr		= 1<<7,
	  CbHiClr		= 1<<16,
	  CbLoClr		= 1<<17,

	  /* enable interrupts for various events */
	  VsyncEna		= 1<<8,
	  Busy2DEna		= 1<<9,
	  BfifoFullEna	= 1<<10,
	  BfifoEmptyEna	= 1<<11,
	  CfifoFullEna	= 1<<12,
	  CfifoEmptyEna	= 1<<13,
	  SubsysBciEna	= 1<<14,
	  CbHiEna		= 1<<24,
	  CbLoEna		= 1<<25,

	  /* 2D graphics engine software reset */
	  GeSoftReset	= 1<<15,

	FifoStatus		= 0x8508,	/* FIFO status: read only */
	  CwbEmpty		= 1<<0,		/* command write buffer empty */
	  CrbEmpty		= 1<<1,		/* command read buffer empty */
	  CobEmpty		= 1<<2,		/* command overflow buffer empty */
	  CfifoEmpty	= 1<<3,		/* command FIFO empty */
	  CwbFull		= 1<<8,		/* command write buffer full */
	  CrbFull		= 1<<9,		/* command read buffer full */
	  CobFull		= 1<<10,	/* command overflow buffer full */
	  CfifoFull		= 1<<11,	/* command FIFO full */

	AdvFunCtl		= 0x850C,	/* Advanced Function Control: read/write */
	  GeEna			= 1<<0,	/* enable 2D/3D engine */
	  /*
	   * according to the manual, BigPixel should be
	   * set when bpp >= 8 (bpp != 4), and then CR50_5-4 are
	   * used to figure out bpp example.  however, it does bad things
	   * to the screen in 8bpp mode.
	   */
	  BigPixel		= 1<<2,		/* 8 or more bpp enhanced mode */
	  LaEna			= 1<<3,		/* linear addressing ena: or'ed with CR58_4 */
	  Mclk_2		= 0<<8,		/* 2D engine clock divide: MCLK/2 */
	  Mclk_4		= 1<<8,		/* " MCLK/4 */
	  Mclk			= 2<<8,		/* " MCLK */
	/* Mclk			= 3<<8,		/* " MCLK */
	  Ic33mhz		= 1<<16,	/* Internal clock 33 MHz (instead of 66) */

	WakeupReg		= 0x8510,	/* Wakeup: read/write */
	  WakeupBit		= 1<<0,	/* wake up: or'ed with 3C3_0 */

	SourceY			= 0x8100,	/* UL corner of bitblt source */
	SourceX			= 0x8102,	/* " */
	RectY			= 0x8100,	/* UL corner of rectangle fill */
	RectX			= 0x8102,	/* " */
	DestY			= 0x8108,	/* UL corner of bitblt dest */
	DestX			= 0x810A,	/* " */
	Height			= 0x8148,	/* bitblt, image xfer rectangle height */
	Width			= 0x814A, 	/* bitblt, image xfer rectangle width */

	StartY			= 0x8100,	/* Line draw: first point*/
	StartX			= 0x8102,	/* " */
	/*
	 * For line draws, the following must be programmed:
	 * axial step constant = 2*min(|dx|,|dy|)
	 * diagonal step constant = 2*[min(|dx|,|dy|) - max(|dx|,|dy|)]
	 * error term = 2*min(|dx|,|dy|) - max(|dx|,|dy| - 1
	 *	[sic] when start X < end X
	 * error term = 2*min(|dx|,|dy|) - max(|dx|,|dy|
	 *  [sic] when start X >= end X
	 */
	AxialStep		= 0x8108,
	DiagonalStep	= 0x810A,
	LineError		= 0x8110,
	MinorLength		= 0x8148,	/* pixel count along minor axis */
	MajorLength		= 0x814A,	/* pixel count along major axis */

	DrawCmd			= 0x8118,	/* Drawing Command: write only */
	  CmdMagic		= 0<<1,
	  AcrossPlane	= 1<<1,		/* across the plane mode */
	  LastPixelOff	= 1<<2,		/* last pixel of line or vector draw not drawn */
	  Radial		= 1<<3,		/* enable radial direction (else axial) */
	  DoDraw		= 1<<4,		/* draw pixels (else only move current pos) */

	  DrawRight		= 1<<5,		/* axial drawing direction: left to right */
	  /* DrawLeft		= 0<<5, */
	  MajorY		= 1<<6,
	  /* MajorX		= 0<<6, */
	  DrawDown		= 1<<7,
	  /* DrawUp		= 0<<7, */
	  Degree0		= 0<<5,		/* drawing direction when Radial */
	  Degree45		= 1<<5,
		/* ... */
	  Degree315		= 7<<5,

	  UseCPUData	= 1<<8,

	  /* image write bus transfer width */
	  Bus8			= 0<<9,
	  Bus16			= 1<<9,
	  /*
	   * in Bus32 mode, doubleword bits beyond the image rect width are
	   * discarded.  each line starts on a new doubleword.
	   * Bus32AP is intended for across-the-plane mode and
	   * rounds to byte boundaries instead.
	   */
	  Bus32			= 2<<9,
	  Bus32AP		= 3<<9,

	  CmdNop		= 0<<13,	/* nop */
	  CmdLine		= 1<<13,	/* draw line */
	  CmdFill		= 2<<13,	/* fill rectangle */
	  CmdBitblt		= 6<<13,	/* bitblt */
	  CmdPatblt		= 7<<13,	/* 8x8 pattern blt */

	  SrcGBD		= 0<<16,
	  SrcPBD		= 1<<16,
	  SrcSBD		= 2<<16,

	  DstGBD		= 0<<18,
	  DstPBD		= 1<<18,
	  DstSBD		= 2<<18,

	/* color sources, controls */
	BgColor			= 0x8120,	/* Background Color: read/write */
	FgColor			= 0x8124,	/* Foreground Color: read/write */
	BitplaneWmask	= 0x8128,	/* Bitplane Write Mask: read/write */
	BitplaneRmask	= 0x812C,	/* Bitplane Read Mask: read/write */
	CmpColor		= 0x8130,	/* Color Compare: read/write */
	BgMix			= 0x8134,
	FgMix			= 0x8136,
	  MixNew		= 7,
	  SrcBg			= 0<<5,
	  SrcFg			= 1<<5,
	  SrcCPU		= 2<<5,
	  SrcDisp		= 3<<5,

	/* clipping rectangle */
	TopScissors		= 0x8138,	/* Top Scissors: write only */
	LeftScissors	= 0x813A,	/* Left Scissors: write only */
	BottomScissors	= 0x813C,	/* Bottom Scissors: write only */
	RightScissors	= 0x813E,	/* Right Scissors: write only */

	/*
	 * Registers with Magic were indirectly accessed in older modes.
	 * It is not clear whether the Magic is necessary.
	 * In the older modes, writes to these registers were pipelined,
	 * so that you had to issue an engine command and wait for engine
	 * idle before reading a write back.  It is not clear if this is
	 * still the case either.
	 */
	PixCtl			= 0x8140,	/* Pixel Control: write only */
	  PixMagic		= 0xA<<12,
	  PixMixFg		= 0<<6,		/* foreground mix register always */
	  PixMixCPU		= 2<<6,		/* CPU data determines mix register */
	  PixMixDisp	= 3<<6,		/* display data determines mix register */

	MfMisc2Ctl		= 0x8142,	/* Multifunction Control Misc. 2: write only */
	  MfMisc2Magic	= 0xD<<12,
	  DstShift		= 0,		/* 3 bits: destination base address in MB */
	  SrcShift		= 4,		/* 3 bits: source base address in MB */
	  WaitFifoEmpty	= 2<<8,		/* wait for write FIFO empty between draws */

	MfMiscCtl		= 0x8144,	/* Multifunction Control Misc: write only */
	  MfMiscMagic	= 0xE<<12,
	  UseHighBits	= 1<<4,		/* select upper 16 bits for 32-bit reg access */
	  ClipInvert	= 1<<5,		/* only touch pixels outside clip rectangle */
	  SkipSame		= 0<<6,		/* ignore pixels with color CmpColor */
	  SkipDifferent	= 1<<7,		/* ignore pixels not color CmpColor */
	  CmpEna		= 1<<8,		/* enable color compare */
	  W32Ena		= 1<<9,		/* enable 32-bit register write */
	  ClipDis		= 1<<11,	/* disable clipping */

	/*
	 * The bitmap descriptor 1 registers contain the starting
	 * address of the bitmap (in bytes).
	 * The bitmap descriptor 2 registesr contain stride (in pixels)
	 * in the lower 16 bits, depth (in bits) in the next 8 bits,
	 * and whether block write is disabled.
	 */
	GBD1			= 0x8168,	/* Global Bitmap Descriptor 1: read/write */
	GBD2			= 0x816C,	/* Global Bitmap Descriptor 2: read/write */
	  /* GBD2-only bits */
	  BDS64			= 1<<0,		/* bitmap descriptor size 64 bits */
	  GBDBciEna		= 1<<3,		/* BCI enable */
	  /* generic BD2 bits */
	  BlockWriteDis	= 1<<28,
	  StrideShift	= 0,
	  DepthShift	= 16,

	PBD1			= 0x8170,	/* Primary Bitmap Descriptor: read/write */
	PBD2			= 0x8174,
	SBD1			= 0x8178,	/* Secondary Bitmap Descriptor: read/write */
	SBD2			= 0x817C,
};

/* mastered data transfer registers */

/* configuration/status registers */
enum {
	XStatus0			= 0x48C00,	/* Status Word 0: read only */
	  /* rev. A silicon differs from rev. B; use AltStatus0 */
	  CBEMaskA		= 0x1FFFF,	/* filled command buffer entries */
	  CBEShiftA		= 0,
	  BciIdleA		= 1<<17,	/* BCI idle */
	  Ge3IdleA		= 1<<18,	/* 3D engine idle */
	  Ge2IdleA		= 1<<19,	/* 2D engine idle */
	  McpIdleA		= 1<<20,	/* motion compensation processor idle */
	  MeIdleA		= 1<<22,	/* master engine idle */
	  PfPendA		= 1<<23,	/* page flip pending */

	  CBEMaskB		= 0x1FFFFF,
	  CBEShiftB		= 0,
	  BciIdleB		= 1<<25,
	  Ge3IdleB		= 1<<26,
	  Ge2IdleB		= 1<<27,
	  McpIdleB		= 1<<28,
	  MeIdleB		= 1<<30,
	  PfPendB		= 1<<31,

	AltStatus0		= 0x48C60,	/* Alternate Status Word 0: read only */
	  CBEMask		= 0x1FFFF,
	  CBEShift		= 0,
	  /* the Savage4 manual says bits 17..23 for these, like Status0 */
	  /* empirically, they are bits 21..26 */
	  BciIdle		= 1<<21,
	  Ge3Idle		= 1<<22,
	  Ge2Idle		= 1<<23,
	  McpIdle		= 1<<24,
	  MeIdle		= 1<<25,
	  PfPend		= 1<<26,

	XStatus1			= 0x48C04,	/* Status Word 1: read only */
	  /* contains event tag 1, event tag 0, both 16 bits */

	XStatus2			= 0x48C08,	/* Status Word 2: read only */
	  ScanMask		= 0x3FF,	/* current scan line */
	  ScanShift		= 0,
	  VRTMask		= 0x7F100,	/* vert retrace count */
	  VRTShift		= 11,

	CbThresh		= 0x48C10,	/* Command Buffer Thresholds: read/write */
	CobOff			= 0x48C14,	/* Command Overflow Buffer: read/write */

	CobPtr			= 0x48C18,	/* Command Overflow Buffer Pointers: read/write */
	  CobEna		= 1<<2,		/* command overflow buffer enable */
	  CobBciEna		= 1<<3,		/* BCI function enable */
	  CbeMask		= 0xFFFF8000,	/* no. of entries in command buffer */
	  CbeShift		= 15,

	AltStatus1		= 0x48C64,	/* Alternate Status Word 1: read onnly */
	  /* contains current texture surface tag, vertex buffer tag */

};

struct {
	ulong idletimeout;
	ulong tostatw[16];
} savagestats;

enum {
	Maxloop = 1<<20
};

static void
savagewaitidle(VGAscr *scr)
{
	long x;
	ulong *statw, mask, goal;

	switch(scr->id){
	case SAVAGE4:
	case PROSAVAGEP:
	case PROSAVAGEK:
	case PROSAVAGE8:
		/* wait for engine idle and FIFO empty */
		statw = (ulong*)((uchar*)scr->mmio+AltStatus0);
		mask = CBEMask | Ge2Idle;
		goal = Ge2Idle;
		break;
	/* case SAVAGEMXMV: ? */
	/* case SAVAGEMX: ? */
	/* case SAVAGEIX: ? */
	case SUPERSAVAGEIXC16:
	case SAVAGEIXMV:
	case SAVAGEMXMV:
		/* wait for engine idle and FIFO empty */
		statw = (ulong*)((uchar*)scr->mmio+XStatus0);
		mask = CBEMaskA | Ge2IdleA;
		goal = Ge2IdleA;
		break;
	default:
		/* 
		 * best we can do: can't print or we'll call ourselves.
		 * savageinit is supposed to not let this happen.
		 */	
		return;
	}

	for(x=0; x<Maxloop; x++)
		if((*statw & mask) == goal)
			return;

	savagestats.tostatw[savagestats.idletimeout++&15] = *statw;
	savagestats.tostatw[savagestats.idletimeout++&15] = (ulong)statw;
}

static int
savagefill(VGAscr *scr, Rectangle r, ulong sval)
{
	uchar *mmio;

	mmio = (uchar*)scr->mmio;

	*(ulong*)(mmio+FgColor) = sval;
	*(ulong*)(mmio+BgColor) = sval;
	*(ulong*)(mmio+BgMix) = SrcFg|MixNew;
	*(ulong*)(mmio+FgMix) = SrcFg|MixNew;
	*(ushort*)(mmio+RectY) = r.min.y;
	*(ushort*)(mmio+RectX) = r.min.x;
	*(ushort*)(mmio+Width) = Dx(r)-1;
	*(ushort*)(mmio+Height) = Dy(r)-1;
	*(ulong*)(mmio+DrawCmd) = CmdMagic | DoDraw | CmdFill | DrawRight | DrawDown;
	savagewaitidle(scr);
	return 1;
}

static int
savagescroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	uchar *mmio;
	ulong cmd;
	Point dp, sp;

	cmd = CmdMagic | DoDraw | CmdBitblt | SrcPBD | DstGBD;

	if(r.min.x <= sr.min.x){
		cmd |= DrawRight;
		dp.x = r.min.x;
		sp.x = sr.min.x;
	}else{
		dp.x = r.max.x-1;
		sp.x = sr.max.x-1;
	}

	if(r.min.y <= sr.min.y){
		cmd |= DrawDown;
		dp.y = r.min.y;
		sp.y = sr.min.y;
	}else{
		dp.y = r.max.y-1;
		sp.y = sr.max.y-1;
	}

	mmio = (uchar*)scr->mmio;

	*(ushort*)(mmio+SourceX) = sp.x;
	*(ushort*)(mmio+SourceY) = sp.y;
	*(ushort*)(mmio+DestX) = dp.x;
	*(ushort*)(mmio+DestY) = dp.y;
	*(ushort*)(mmio+Width) = Dx(r)-1;
	*(ushort*)(mmio+Height) = Dy(r)-1;
	*(ulong*)(mmio+BgMix) = SrcDisp|MixNew;
	*(ulong*)(mmio+FgMix) = SrcDisp|MixNew;
	*(ulong*)(mmio+DrawCmd) = cmd;
	savagewaitidle(scr);
	return 1;
}

static void
savageblank(VGAscr*, int blank)
{
	uchar seqD;

	/*
	 * Will handle DPMS to monitor
	 */
	vgaxo(Seqx, 8, vgaxi(Seqx,8)|0x06);
	seqD = vgaxi(Seqx, 0xD);
	seqD &= 0x03;
	if(blank)
		seqD |= 0x50;
	vgaxo(Seqx, 0xD, seqD);

	/*
	 * Will handle LCD
	 */
	if(blank)
		vgaxo(Seqx, 0x31, vgaxi(Seqx, 0x31) & ~0x10);
	else
		vgaxo(Seqx, 0x31, vgaxi(Seqx, 0x31) | 0x10);
}


void
savageinit(VGAscr *scr)
{
	uchar *mmio;
	ulong bd;

	/* if you add chip IDs here be sure to update savagewaitidle */
	switch(scr->id){
	case SAVAGE4:
	case PROSAVAGEP:
	case PROSAVAGEK:
	case PROSAVAGE8:
	case SAVAGEIXMV:
	case SUPERSAVAGEIXC16:
	case SAVAGEMXMV:
		break;
	default:
		print("unknown savage %.4lux\n", scr->id);
		return;
	}

	mmio = (uchar*)scr->mmio;
	if(mmio == nil) {
		print("savageinit: no mmio\n");
		return;
	}

	/* 2D graphics engine software reset */
	*(ushort*)(mmio+SubsystemCtl) = GeSoftReset;
	delay(2);
	*(ushort*)(mmio+SubsystemCtl) = 0;
	savagewaitidle(scr);

	/* disable BCI as much as possible */
	*(ushort*)(mmio+CobPtr) &= ~CobBciEna;
	*(ushort*)(mmio+GBD2) &= ~GBDBciEna;
	savagewaitidle(scr);

	/* enable 32-bit writes, disable clipping */
	*(ushort*)(mmio+MfMiscCtl) = MfMiscMagic|W32Ena|ClipDis;
	savagewaitidle(scr);

	/* enable all read, write planes */
	*(ulong*)(mmio+BitplaneRmask) = ~0;
	*(ulong*)(mmio+BitplaneWmask) = ~0;
	savagewaitidle(scr);

	/* turn on linear access, 2D engine */
	*(ulong*)(mmio+AdvFunCtl) |= GeEna|LaEna;
	savagewaitidle(scr);

	/* set bitmap descriptors */
	bd = (scr->gscreen->depth<<DepthShift) |
		(Dx(scr->gscreen->r)<<StrideShift) | BlockWriteDis
		| BDS64;

	*(ulong*)(mmio+GBD1) = 0;
	*(ulong*)(mmio+GBD2) = bd;

	*(ulong*)(mmio+PBD1) = 0;
	*(ulong*)(mmio+PBD2) = bd;

	*(ulong*)(mmio+SBD1) = 0;
	*(ulong*)(mmio+SBD2) = bd;

	/*
	 * For some reason, the GBD needs to get programmed twice,
	 * once before the PBD, SBD, and once after.
	 * This empirically makes it get set right.
	 * I would like to better understand the ugliness
	 * going on here.
	 */
	*(ulong*)(mmio+GBD1) = 0;
	*(ulong*)(mmio+GBD2) = bd;
	*(ushort*)(mmio+GBD2+2) = bd>>16;
	savagewaitidle(scr);

	scr->fill = savagefill;
	scr->scroll = savagescroll;
	scr->blank = savageblank;
	hwblank = 0;
}
