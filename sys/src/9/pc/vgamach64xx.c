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

char Eunsupportedformat[] = "unsupported video format";
char Enotconfigured[] = "device not configured";

#define SCALE_ZERO_EXTEND           	0x0
#define SCALE_DYNAMIC               		0x1
#define SCALE_RED_TEMP_6500K        	0x0
#define SCALE_RED_TEMP_9800K        	0x2
#define SCALE_HORZ_BLEND            	0x0
#define SCALE_HORZ_REP              		0x4
#define SCALE_VERT_BLEND            		0x0
#define SCALE_VERT_REP              		0x8
#define SCALE_BANDWIDTH_NORMAL     0x0
#define SCALE_BANDWIDTH_EXCEEDED  0x4000000
#define SCALE_BANDWIDTH_RESET       	0x4000000
#define SCALE_CLK_ACTIVITY          	0x0
#define SCALE_CLK_CONTINUOUS        	0x20000000
#define OVERLAY_DISABLE             		0x0
#define OVERLAY_ENABLE              		0x40000000
#define SCALE_DISABLE               		0x0
#define SCALE_ENABLE                		0x80000000

#define SCALER_FRAME_READ_MODE_FULL 	0x0
#define SCALER_BUF_MODE_SINGLE      		0x0
#define SCALER_BUF_MODE_DOUBLE      		0x40000
#define SCALER_BUF_NEXT_0           		0x0
#define SCALER_BUF_NEXT_1           		0x80000
#define SCALER_BUF_STATUS_0         		0x0
#define SCALER_BUF_STATUS_1         		0x100000

#define OVERLAY_MIX_G_CMP           		0x0
#define OVERLAY_MIX_ALWAYS_G        		0x100
#define OVERLAY_MIX_ALWAYS_V        		0x200
#define OVERLAY_MIX_NOT_G           		0x300
#define OVERLAY_MIX_NOT_V           		0x400
#define OVERLAY_MIX_G_XOR_V         		0x500
#define OVERLAY_MIX_NOT_G_XOR_V     	0x600
#define OVERLAY_MIX_V_CMP           		0x700
#define OVERLAY_MIX_NOT_G_OR_NOT_V	0x800
#define OVERLAY_MIX_G_OR_NOT_V      	0x900
#define OVERLAY_MIX_NOT_G_OR_V      	0xA00
#define OVERLAY_MIX_G_OR_V          		0xB00
#define OVERLAY_MIX_G_AND_V         		0xC00
#define OVERLAY_MIX_NOT_G_AND_V     	0xD00
#define OVERLAY_MIX_G_AND_NOT_V     	0xE00
#define OVERLAY_MIX_NOT_G_AND_NOT_V 	0xF00
#define OVERLAY_EXCLUSIVE_NORMAL    	0x0
#define OVERLAY_EXCLUSIVE_V_ONLY    	0x80000000

#define VIDEO_IN_8BPP               			0x2
#define VIDEO_IN_16BPP              			0x4
#define VIDEO_IN_32BPP              			0x6
#define VIDEO_IN_VYUY422            			0xB         		/*16 bpp */
#define VIDEO_IN_YVYU422            			0xC         		/* 16 bpp */
#define SCALE_IN_15BPP              			0x30000     	/* aRGB 1555 */
#define SCALE_IN_16BPP              			0x40000     	/* RGB 565 */
#define SCALE_IN_32BPP              			0x60000     	/* aRGB 8888 */
#define SCALE_IN_YUV9               			0x90000     	/* planar */
#define SCALE_IN_YUV12              			0xA0000     	/* planar */
#define SCALE_IN_VYUY422            			0xB0000     	/* 16 bpp */
#define SCALE_IN_YVYU422            			0xC0000     	/* 16 bpp */
#define HOST_YUV_APERTURE_UPPER     		0x0
#define HOST_YUV_APERTURE_LOWER     	0x20000000
#define HOST_MEM_MODE_Y             		0x40000000
#define HOST_MEM_MODE_U             		0x80000000
#define HOST_MEM_MODE_V             		0xC0000000
#define HOST_MEM_MODE_NORMAL     		HOST_YUV_APERTURE_UPPER 

static Chan *ovl_chan;	/* Channel of controlling process */
static int ovl_width;		/* Width of input overlay buffer */
static int ovl_height;		/* Height of input overlay buffer */
static int ovl_format;	/* Overlay format */
static ulong ovl_fib;		/* Frame in bytes */

enum {
	 VTGTB1S1        = 0x01,            //  Asic description for VTB1S1 and GTB1S1.
	 VT4GTIIC        	= 0x3A,            // asic descr for VT4 and RAGE IIC
	 GTB1U1          	= 0x19,            //  Asic description for GTB1U1.
	 GTB1S2          	= 0x41,            //  Asic description for GTB1S2.
	 GTB2U1          	= 0x1A,
	 GTB2U2          	= 0x5A,
	 GTB2U3          	= 0x9A,
	 GTIIIC1U1       	= 0x1B,            // 3D RAGE PRO asic descrp.
	 GTIIIC1U2       	= 0x5B,            // 3D RAGE PRO asic descrp.
	 GTIIIC2U1       	= 0x1C,            // 3D RAGE PRO asic descrp.
	 GTIIIC2U2       	= 0x5C,           // 3D RAGE PRO asic descrp.
	 GTIIIC2U3       	= 0x7C,            // 3D RAGE PRO asic descrp.
	 GTBC            	= 0x3A,            // 3D RAGE IIC asic descrp.
	 LTPRO           	= 0x9C,            // 3D RAGE LT PRO
};

/*
 * ATI Mach64(CT|ET|G*|V*|L*).
 */
typedef struct Mach64types Mach64types;
struct Mach64types {
	ushort 	m64_id;			/* Chip ID */
	int 		m64_vtgt;		/* Is this a VT or GT chipset? */
	ulong	m64_ovlclock;		/* Max. overlay clock frequency */
	int		m64_pro;			/* Is this a PRO? */
};

static ulong mach64refclock;
static Mach64types *mach64type;
static int mach64revb;			/* Revision B or greater? */
static ulong mach64overlay;		/* Overlay buffer */

static Mach64types mach64s[] = {
	('C'<<8)|'T',	0,	1350000, /*?*/	0,	/* 4354: CT */
	('E'<<8)|'T',	0,	1350000, /*?*/	0,	/* 4554: ET */
	('G'<<8)|'B',	1,	1250000,		1, 	/* 4742: 264GT PRO */
	('G'<<8)|'D',	1,	1250000,		1, 	/* 4744: 264GT PRO */
	('G'<<8)|'I',	1,	1250000,		1, 	/* 4749: 264GT PRO */
	('G'<<8)|'M',	0,	1350000,		0,	/* 474D: Rage XL */
	('G'<<8)|'P',	1,	1250000,		1, 	/* 4750: 264GT PRO */
	('G'<<8)|'Q',	1,	1250000,		1,	/* 4751: 264GT PRO */
	('G'<<8)|'R',	1,	1250000,		1,	/* 4752: */
	('G'<<8)|'T',	1,	800000,		0,	/* 4754: 264GT[B] */
	('G'<<8)|'U',	1,	1000000,		0,	/* 4755: 264GT DVD */
	('G'<<8)|'V',	1,	1000000,		0,	/* 4756: Rage2C */
	('G'<<8)|'Z',	1,	1000000,		0,	/* 475A: Rage2C */
	('V'<<8)|'T',	1,	800000,		0,	/* 5654: 264VT/GT/VTB */
	('V'<<8)|'U',	1,	800000,		0,	/* 5655: 264VT3 */
	('V'<<8)|'V',	1,	1000000,		0,	/* 5656: 264VT4 */
	('L'<<8)|'B',	0,	1350000,		1,	/* 4C42: Rage LTPro AGP */
	('L'<<8)|'I',		0,	1350000,		0,	/* 4C49: Rage LTPro AGP */
	('L'<<8)|'M',	0,	1350000,		0,	/* 4C4D: Rage Mobility */
	('L'<<8)|'P',	0,	1350000,		1,	/* 4C50: 264LT PRO */
};


static int hwfill(VGAscr*, Rectangle, ulong);
static int hwscroll(VGAscr*, Rectangle, Rectangle);
static void initengine(VGAscr*);

static Pcidev*
mach64xxpci(void)
{
	Pcidev *p;
	int i;

	if((p = pcimatch(nil, 0x1002, 0)) == nil)
		return nil;

	for (i = 0; i != nelem(mach64s); i++)
		if (mach64s[i].m64_id == p->did) {
			mach64type = &mach64s[i];
			return p;
		}
	return nil;
}

static void
mach64xxenable(VGAscr* scr)
{
	Pcidev *p;

	/*
	 * Only once, can't be disabled for now.
	 */
	if(scr->io)
		return;
	if(p = mach64xxpci()){
		scr->id = p->did;

		/*
		 * The CT doesn't always have the I/O base address
		 * in the PCI base registers. There is a way to find
		 * it via the vendor-specific PCI config space but
		 * this will do for now.
		 */
		scr->io = p->mem[1].bar & ~0x03;

		if(scr->io == 0)
			scr->io = 0x2EC;
	}
}

static ulong
mach64xxlinear(VGAscr* scr, int* size, int* align)
{
	ulong aperture, osize, oaperture;
	int i, oapsize, wasupamem;
	Pcidev *p;

	osize = *size;
	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	if(p = mach64xxpci()){
		for(i=0; i<nelem(p->mem); i++){
			if(p->mem[i].size >= *size
			&& ((p->mem[i].bar & ~0x0F) & (*align-1)) == 0)
				break;
		}
		if(i >= nelem(p->mem)){
			print("vgamach64xx: aperture not found\n");
			return 0;
		}
		aperture = p->mem[i].bar & ~0x0F;
		*size = p->mem[i].size;
	}
	else
		aperture = 0;

	if(wasupamem)
		upafree(oaperture, oapsize);
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0))
			scr->isupamem = 1;
	}
	else
		scr->isupamem = 1;

	scr->mmio = KADDR(aperture+osize-0x400);
	if(oaperture && oaperture != aperture)
		print("warning (BUG): redefinition of aperture does not change mach64mmio segment\n");
	addvgaseg("mach64mmio", aperture+osize-BY2PG, BY2PG);
	addvgaseg("mach64screen", aperture, osize);

	return aperture;
}

enum {
	CrtcOffPitch	= 0x05,
	CrtcGenCtl	= 0x07,
	CurClr0		= 0x0B,		/* I/O Select */
	CurClr1		= 0x0C,
	CurOffset	= 0x0D,
	CurHVposn	= 0x0E,
	CurHVoff	= 0x0F,
	BusCntl	= 0x13,
	GenTestCntl	= 0x19,

	CrtcHsyncDis	= 0x04,
	CrtcVsyncDis	= 0x08,

	ContextMask	= 0x100,	/* not accessible via I/O */
	FifoStat,
	GuiStat,
	DpFrgdClr,
	DpBkgdClr,
	DpWriteMask,
	DpMix,
	DpPixWidth,
	DpSrc,
	ClrCmpCntl,
	GuiTrajCntl,
	ScLeftRight,
	ScTopBottom,
	DstOffPitch,
	DstYX,
	DstHeightWidth,
	DstCntl,
	DstHeight,
	DstBresErr,
	DstBresInc,
	DstBresDec,
	SrcCntl,
	SrcHeight1Width1,
	SrcHeight2Width2,
	SrcYX,
	SrcWidth1,
	SrcYXstart,
	HostCntl,
	PatReg0,
	PatReg1,
	PatCntl,
	ScBottom,
	ScLeft,
	ScRight,
	ScTop,
	ClrCmpClr,
	ClrCmpMask,
	DpChainMask,
	SrcOffPitch,	
	LcdIndex,
	LcdData,
	ClockCntl,
	OverlayScaleCntl,
	ConfigChipId,
	Buf0Pitch,
	ScalerBuf0Pitch,
	CaptureConfig,
	OverlayKeyCntl,
	ScalerColourCntl,
	ScalerHCoef0,
	ScalerHCoef1,
	ScalerHCoef2,
	ScalerHCoef3,
	ScalerHCoef4,
	VideoFormat,
	Buf0Offset,
	ScalerBuf0Offset,
	CrtcGenCntl,
	OverlayScaleInc,
	OverlayYX,
	OverlayYXEnd,
	ScalerHeightWidth,
	HTotalDisp,
	VTotalDisp,
};

enum {
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

#define Bank1			(-0x100)		/* 1KB */

static int mmoffset[] = {
	[HTotalDisp]		0x00,
	[VTotalDisp]		0x02,
	[CrtcOffPitch]		0x05,
	[CrtcGenCntl]		0x07,
	[CurClr0]			0x18,
	[CurClr1]			0x19,
	[CurOffset]		0x1A,
	[CurHVposn]		0x1B,
	[CurHVoff]		0x1C,
	[ClockCntl]		0x24,
	[BusCntl]			0x28,
	[LcdIndex]		0x29,
	[LcdData]			0x2A,
	[GenTestCntl]		0x34,
	[ConfigChipId]		0x38,
	[DstOffPitch]		0x40,
	[DstYX]			0x43,
	[DstHeight]		0x45,
	[DstHeightWidth]	0x46,
	[DstBresErr]		0x49,
	[DstBresInc]		0x4A,
	[DstBresDec]		0x4B,
	[DstCntl]			0x4C,
	[SrcOffPitch]		0x60,
	[SrcYX]			0x63,
	[SrcWidth1]		0x64,
	[SrcYXstart]		0x69,
	[SrcHeight1Width1]	0x66,
	[SrcHeight2Width2]	0x6C,
	[SrcCntl]			0x6D,
	[HostCntl]			0x90,
	[PatReg0]			0xA0,
	[PatReg1]			0xA1,
	[PatCntl]			0xA2,
	[ScLeft]			0xA8,
	[ScRight]			0xA9,
	[ScLeftRight]		0xAA,
	[ScTop]			0xAB,
	[ScBottom] 		0xAC,
	[ScTopBottom]		0xAD,
	[DpBkgdClr]		0xB0,
	[DpFrgdClr]		0xB1,
	[DpWriteMask]		0xB2,
	[DpChainMask]		0xB3,
	[DpPixWidth]		0xB4,
	[DpMix]			0xB5,
	[DpSrc]			0xB6,
	[ClrCmpClr]		0xC0,
	[ClrCmpMask]		0xC1,
	[ClrCmpCntl]		0xC2,
	[FifoStat]			0xC4,
	[ContextMask]		0xC8,
	[GuiTrajCntl]		0xCC,
	[GuiStat]			0xCE,

	/* Bank1 */
	[OverlayYX]		Bank1 + 0x00,
	[OverlayYXEnd]		Bank1 + 0x01,
	[OverlayKeyCntl]	Bank1 + 0x06,
	[OverlayScaleInc]	Bank1 + 0x08,
	[OverlayScaleCntl]	Bank1 + 0x09,
	[ScalerHeightWidth]	Bank1 + 0x0A,
	[ScalerBuf0Offset]	Bank1 + 0x0D,
	[ScalerBuf0Pitch]	Bank1 + 0x0F,
	[VideoFormat]		Bank1 + 0x12,
	[CaptureConfig]	Bank1 + 0x14,
	[Buf0Offset]		Bank1 + 0x20,
	[Buf0Pitch]		Bank1 + 0x23,
	[ScalerColourCntl]	Bank1 + 0x54,
	[ScalerHCoef0]		Bank1 + 0x55,
	[ScalerHCoef1]		Bank1 + 0x56,
	[ScalerHCoef2]		Bank1 + 0x57,
	[ScalerHCoef3]		Bank1 + 0x58,
	[ScalerHCoef4]		Bank1 + 0x59,
};

static ulong
ior32(VGAscr* scr, int r)
{
	if(scr->io == 0x2EC || scr->io == 0x1C8)
		return inl((r<<10)+scr->io);
	if(r >= 0x100 && scr->mmio != nil)
		return scr->mmio[mmoffset[r]];
	return inl((mmoffset[r]<<2)+scr->io);
}

static void
iow32(VGAscr* scr, int r, ulong l)
{
	if(scr->io == 0x2EC || scr->io == 0x1C8)
		outl(((r)<<10)+scr->io, l);
	else if(r >= 0x100 && scr->mmio != nil)
		scr->mmio[mmoffset[r]] = l;
	else
		outl((mmoffset[r]<<2)+scr->io, l);
}

static ulong
lcdr32(VGAscr *scr, ulong r)
{
	ulong or;

	or = ior32(scr, LcdIndex);
	iow32(scr, LcdIndex, (or&~0x0F) | (r&0x0F));
	return ior32(scr, LcdData);
}

static void
lcdw32(VGAscr *scr, ulong r, ulong v)
{
	ulong or;

	or = ior32(scr, LcdIndex);
	iow32(scr, LcdIndex, (or&~0x0F) | (r&0x0F));
	iow32(scr, LcdData, v);
}

static void
mach64xxcurdisable(VGAscr* scr)
{
	ulong r;

	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);
}

static void
mach64xxcurload(VGAscr* scr, Cursor* curs)
{
	uchar *p;
	int i, y;
	ulong c, s, m, r;

	/*
	 * Disable the cursor.
	 */
	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);

	p = KADDR(scr->aperture);
	p += scr->storage;

	/*
	 * Initialise the 64x64 cursor RAM array.
	 * The cursor mode gives the following truth table:
	 *	p1 p0	colour
	 *	 0  0	Cursor Colour 0
	 *	 0  1	Cursor Colour 1
	 *	 1  0	Transparent
	 *	 1  1	Complement
	 * Put the cursor into the top-right of the 64x64 array.
	 */
	for(y = 0; y < 16; y++){
		for(i = 0; i < (64-16)/8; i++){
			*p++ = 0xAA;
			*p++ = 0xAA;
		}

		c = (curs->clr[2*y]<<8)|curs->clr[y*2 + 1];
		s = (curs->set[2*y]<<8)|curs->set[y*2 + 1];

		m = 0x00000000;
		for(i = 0; i < 16; i++){
			if(s & (1<<(15-i)))
				m |= 0x01<<(2*i);
			else if(c & (1<<(15-i))){
				/* nothing to do */
			}
			else
				m |= 0x02<<(2*i);
		}
		*p++ = m;
		*p++ = m>>8;
		*p++ = m>>16;
		*p++ = m>>24;
	}
	memset(p, 0xAA, (64-16)*16);

	/*
	 * Set the cursor hotpoint and enable the cursor.
	 */
	scr->offset = curs->offset;
	iow32(scr, GenTestCntl, 0x80|r);
}

static int
ptalmostinrect(Point p, Rectangle r)
{
	return p.x>=r.min.x && p.x<=r.max.x &&
	       p.y>=r.min.y && p.y<=r.max.y;
}

/*
 * If necessary, translate the rectangle physr
 * some multiple of [dx dy] so that it includes p.
 * Return 1 if the rectangle changed.
 */
static int
screenpan(Point p, Rectangle *physr, int dx, int dy)
{
	int d;

	if(ptalmostinrect(p, *physr))
		return 0;

	if(p.y < physr->min.y){
		d = physr->min.y - (p.y&~(dy-1));
		physr->min.y -= d;
		physr->max.y -= d;
	}
	if(p.y > physr->max.y){
		d = ((p.y+dy-1)&~(dy-1)) - physr->max.y;
		physr->min.y += d;
		physr->max.y += d;
	}

	if(p.x < physr->min.x){
		d = physr->min.x - (p.x&~(dx-1));
		physr->min.x -= d;
		physr->max.x -= d;
	}
	if(p.x > physr->max.x){
		d = ((p.x+dx-1)&~(dx-1)) - physr->max.x;
		physr->min.x += d;
		physr->max.x += d;
	}
	return 1;
}

static int
mach64xxcurmove(VGAscr* scr, Point p)
{
	int x, xo, y, yo;
	int dx;
	ulong off, pitch;

	/*
	 * If the point we want to display is outside the current
	 * screen rectangle, pan the screen to display it.
	 *
	 * We have to move in 64-bit chunks.
	 */
	if(scr->gscreen->depth == 24)
		dx = (64*3)/24;
	else
		dx = 64 / scr->gscreen->depth;

	if(panning && screenpan(p, &physgscreenr, dx, 1)){
		off = (physgscreenr.min.y*Dx(scr->gscreen->r)+physgscreenr.min.x)/dx;
		pitch = Dx(scr->gscreen->r)/8;
		iow32(scr, CrtcOffPitch, (pitch<<22)|off);
	}

	p.x -= physgscreenr.min.x;
	p.y -= physgscreenr.min.y;

	/*
	 * Mustn't position the cursor offscreen even partially,
	 * or it disappears. Therefore, if x or y is -ve, adjust the
	 * cursor presets instead. If y is negative also have to
	 * adjust the starting offset.
	 */
	if((x = p.x+scr->offset.x) < 0){
		xo = x;
		x = 0;
	}
	else
		xo = 0;
	if((y = p.y+scr->offset.y) < 0){
		yo = y;
		y = 0;
	}
	else
		yo = 0;

	iow32(scr, CurHVoff, ((64-16-yo)<<16)|(64-16-xo));
	iow32(scr, CurOffset, scr->storage/8 + (-yo*2));
	iow32(scr, CurHVposn, (y<<16)|x);

	return 0;
}

static void
mach64xxcurenable(VGAscr* scr)
{
	ulong r, storage;

	mach64xxenable(scr);
	if(scr->io == 0)
		return;

	r = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, r & ~0x80);

	iow32(scr, CurClr0, (Pwhite<<24)|(Pwhite<<16)|(Pwhite<<8)|Pwhite);
	iow32(scr, CurClr1, (Pblack<<24)|(Pblack<<16)|(Pblack<<8)|Pblack);

	/*
	 * Find a place for the cursor data in display memory.
	 * Must be 64-bit aligned.
	 */
	storage = (scr->gscreen->width*BY2WD*scr->gscreen->r.max.y+7)/8;
	iow32(scr, CurOffset, storage);
	scr->storage = storage*8;

	/*
	 * Cursor goes in the top right corner of the 64x64 array
	 * so the horizontal and vertical presets are 64-16.
	 */
	iow32(scr, CurHVposn, (0<<16)|0);
	iow32(scr, CurHVoff, ((64-16)<<16)|(64-16));

	/*
	 * Load, locate and enable the 64x64 cursor.
	 */
	mach64xxcurload(scr, &arrow);
	mach64xxcurmove(scr, ZP);
	iow32(scr, GenTestCntl, 0x80|r);
}

static void
waitforfifo(VGAscr *scr, int entries)
{
	int x;

	x = 0;
	while((ior32(scr, FifoStat)&0xFF) > (0x8000>>entries) && x++ < 1000000)
		;
	if(x >= 1000000)
		iprint("fifo %d stat %.8lux %.8lux scrio %.8lux mmio %p scr %p pc %luX\n", entries, ior32(scr, FifoStat), scr->mmio[mmoffset[FifoStat]], scr->io, scr->mmio, scr, getcallerpc(&scr));
}

static void
waitforidle(VGAscr *scr)
{
	int x;

	waitforfifo(scr, 16);
	x = 0;
	while((ior32(scr, GuiStat)&1) && x++ < 1000000)
		;
	if(x >= 1000000)
		iprint("idle stat %.8lux %.8lux scrio %.8lux mmio %p scr %p pc %luX\n", ior32(scr, GuiStat), scr->mmio[mmoffset[GuiStat]], scr->io, scr->mmio, scr, getcallerpc(&scr));
}

static void
resetengine(VGAscr *scr)
{
	ulong x;
	x = ior32(scr, GenTestCntl);
	iow32(scr, GenTestCntl, x&~0x100);
	iow32(scr, GenTestCntl, x|0x100);
	iow32(scr, BusCntl, ior32(scr, BusCntl)|0x00A00000);
}

static void
init_overlayclock(VGAscr *scr)
{
	uchar *cc, save, pll_ref_div, pll_vclk_cntl, vclk_post_div, 
			vclk_fb_div, ecp_div;
	int i;
	ulong dotclock;

	/* Taken from GLX */
	/* Get monitor dotclock, check for Overlay Scaler clock limit */
 	cc = (uchar *)&scr->mmio[mmoffset[ClockCntl]];
  	save = cc[1]; i = cc[0] & 3;
  	cc[1] = 2<<2; pll_ref_div = cc[2];
  	cc[1] = 5<<2; pll_vclk_cntl = cc[2];
  	cc[1] = 6<<2; vclk_post_div = (cc[2]>>(i+i)) & 3;
  	cc[1] = (7+i)<<2; vclk_fb_div = cc[2];

	dotclock = 2 * mach64refclock * vclk_fb_div / 
			(pll_ref_div * (1 << vclk_post_div));
	/* ecp_div: 0=dotclock, 1=dotclock/2, 2=dotclock/4 */
  	ecp_div = dotclock / mach64type->m64_ovlclock;
  	if (ecp_div>2) ecp_div = 2;

  	/* Force a scaler clock factor of 1 if refclock *
   	  * is unknown (VCLK_SRC not PLLVCLK)  */
  	if ((pll_vclk_cntl & 0x03) != 0x03) 
		ecp_div = 0;
  	if ((pll_vclk_cntl & 0x30) != ecp_div<<4) {
    		cc[1] = (5<<2)|2;
    		cc[2] = (pll_vclk_cntl&0xCF) | (ecp_div<<4);
	}

  	/* Restore PLL Register Index */
  	cc[1] = save;
}

static void
initengine(VGAscr *scr)
{
	ulong pitch;
	uchar *bios;
	ushort table;

	pitch = Dx(scr->gscreen->r)/8;
	if(scr->gscreen->depth == 24)
		pitch *= 3;

	resetengine(scr);
	waitforfifo(scr, 14);
	iow32(scr, ContextMask, ~0);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstYX, 0);
	iow32(scr, DstHeight, 0);
	iow32(scr, DstBresErr, 0);
	iow32(scr, DstBresInc, 0);
	iow32(scr, DstBresDec, 0);
	iow32(scr, DstCntl, 0x23);
	iow32(scr, SrcOffPitch, pitch<<22);
	iow32(scr, SrcYX, 0);
	iow32(scr, SrcHeight1Width1, 1);
	iow32(scr, SrcYXstart, 0);
	iow32(scr, SrcHeight2Width2, 1);
	iow32(scr, SrcCntl, 0x01);

	waitforfifo(scr, 13);
	iow32(scr, HostCntl, 0);
	iow32(scr, PatReg0, 0);
	iow32(scr, PatReg1, 0);
	iow32(scr, PatCntl, 0);
	iow32(scr, ScLeft, 0);
	iow32(scr, ScTop, 0);
	iow32(scr, ScBottom, 0xFFFF);
	iow32(scr, ScRight, 0xFFFF);
	iow32(scr, DpBkgdClr, 0);
	iow32(scr, DpFrgdClr, ~0);
	iow32(scr, DpWriteMask, ~0);
	iow32(scr, DpMix, 0x70003);
	iow32(scr, DpSrc, 0x00010100);

	waitforfifo(scr, 3);
	iow32(scr, ClrCmpClr, 0);
	iow32(scr, ClrCmpMask, ~0);
	iow32(scr, ClrCmpCntl, 0);

	waitforfifo(scr, 2);
	switch(scr->gscreen->depth){
	case 8:
	case 24:	/* [sic] */
		iow32(scr, DpPixWidth, 0x00020202);
		iow32(scr, DpChainMask, 0x8080);
		break;
	case 16:
		iow32(scr, DpPixWidth, 0x00040404);
		iow32(scr, DpChainMask, 0x8410);
		break;
	case 32:
		iow32(scr, DpPixWidth, 0x00060606);
		iow32(scr, DpChainMask, 0x8080);
		break;
	}

	/* Get the base freq from the BIOS */
	bios  = KADDR(0xC000);
	table = *(ushort *)(bios + 0x48);
	table = *(ushort *)(bios + table + 0x10);
	switch (*(ushort *)(bios + table + 0x08)) {
      	case 2700: 
		mach64refclock = 270000; 
		break;
      	case 2863: 
      	case 2864: 
		mach64refclock = 286363; 
		break;
      	case 2950: 
		mach64refclock = 294989; 
		break;
    	case 1432: 
	default:
		mach64refclock = 143181; 
		break ;	
	}
	
	/* Figure out which revision this chip is */
	switch ((scr->mmio[mmoffset[ConfigChipId]] >> 24) & 0xFF) {
	case VTGTB1S1:
	case GTB1U1:
	case GTB1S2:
	case GTB2U1:
	case GTB2U2:
	case GTB2U3:
	case GTBC:
	case GTIIIC1U1:
	case GTIIIC1U2:
	case GTIIIC2U1:
	case GTIIIC2U2: 
	case GTIIIC2U3: 
	case LTPRO:
			mach64revb = 1;
			break;
	default: 
			mach64revb = 0;
			break;
	}

	waitforidle(scr);
}

static int
mach64hwfill(VGAscr *scr, Rectangle r, ulong sval)
{
	ulong pitch;
	ulong ctl;

if(drawdebug)
	iprint("hwfill %R val %lux...\n", r, sval);

	/* shouldn't happen */
	if(scr->io == 0x2EC || scr->io == 0x1C8 || scr->io == 0)
		return 0;

	pitch = Dx(scr->gscreen->r)/8;
	ctl = 1|2;	/* left-to-right, top-to-bottom */
	if(scr->gscreen->depth == 24){
		r.min.x *= 3;
		r.max.x *= 3;
		pitch *= 3;
		ctl |= (1<<7)|(((r.min.x/4)%6)<<8);
	}

	waitforfifo(scr, 11);
	iow32(scr, DpFrgdClr, sval);
	iow32(scr, DpWriteMask, 0xFFFFFFFF);
	iow32(scr, DpMix, 0x00070003);
	iow32(scr, DpSrc, 0x00000111);
	iow32(scr, ClrCmpCntl, 0x00000000);
	iow32(scr, ScLeftRight, 0x1FFF0000);
	iow32(scr, ScTopBottom, 0x1FFF0000);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstCntl, ctl);
	iow32(scr, DstYX, (r.min.x<<16)|r.min.y);
	iow32(scr, DstHeightWidth, (Dx(r)<<16)|Dy(r));

	waitforidle(scr);
	return 1;
}

static int
mach64hwscroll(VGAscr *scr, Rectangle r, Rectangle sr)
{
	ulong pitch;
	Point dp, sp;
	ulong ctl;
	int dx, dy;

	dx = Dx(r);
	dy = Dy(r);
	pitch = Dx(scr->gscreen->r)/8;
	if(scr->gscreen->depth == 24){
		dx *= 3;
		pitch *= 3;
		r.min.x *= 3;
		sr.min.x *= 3;
	}

	ctl = 0;
	if(r.min.x <= sr.min.x){
		ctl |= 1;
		dp.x = r.min.x;
		sp.x = sr.min.x;
	}else{
		dp.x = r.min.x+dx-1;
		sp.x = sr.min.x+dx-1;
	}

	if(r.min.y <= sr.min.y){
		ctl |= 2;
		dp.y = r.min.y;
		sp.y = sr.min.y;
	}else{
		dp.y = r.min.y+dy-1;
		sp.y = sr.min.y+dy-1;
	}

	if(scr->gscreen->depth == 24)
		ctl |= (1<<7)|(((dp.x/4)%6)<<8);

	waitforfifo(scr, 6);
	iow32(scr, ScLeftRight, 0x1FFF0000);
	iow32(scr, ScTopBottom, 0x1FFF0000);
	iow32(scr, DpWriteMask, 0xFFFFFFFF);
	iow32(scr, DpMix, 0x00070003);
	iow32(scr, DpSrc, 0x00000300);
	iow32(scr, ClrCmpCntl, 0x00000000);

	waitforfifo(scr, 8);
	iow32(scr, SrcOffPitch, pitch<<22);
	iow32(scr, SrcCntl, 0x00000000);
	iow32(scr, SrcYX, (sp.x<<16)|sp.y);
	iow32(scr, SrcWidth1, dx);
	iow32(scr, DstOffPitch, pitch<<22);
	iow32(scr, DstCntl, ctl);

	iow32(scr, DstYX, (dp.x<<16)|dp.y);
	iow32(scr, DstHeightWidth, (dx<<16)|dy);

	waitforidle(scr);

	return 1;
}

/*
 * This should work, but doesn't.
 * It messes up the screen timings for some reason.
 */
static void
mach64blank(VGAscr *scr, int blank)
{
	ulong ctl;

	ctl = ior32(scr, CrtcGenCtl) & ~(CrtcHsyncDis|CrtcVsyncDis);
	if(blank)
		ctl |= CrtcHsyncDis|CrtcVsyncDis;
	iow32(scr, CrtcGenCtl, ctl);
}

/*
 * We squirrel away whether the LCD and/or CRT were
 * on when we were called to blank the screen, and
 * restore the old state.  If we are called to blank the
 * screen when it is already blank, we don't update the state.
 * Such a call sequence should not happen, though.
 *
 * We could try forcing the chip into power management
 * mode instead, but I'm not sure how that would interact
 * with screen updates going on while the screen is blanked.
 */
static void
mach64lcdblank(VGAscr *scr, int blank)
{
	static int crtlcd;
	ulong x;

	if(blank) {
		x = lcdr32(scr, LCD_GenCtrl);
		if(x & 3) {
			crtlcd = x & 3;
			lcdw32(scr, LCD_GenCtrl,  x&~3);
		}
	} else {
		if(crtlcd == 0)
			crtlcd = 2;	/* lcd only */
		x = lcdr32(scr, LCD_GenCtrl);
		lcdw32(scr, LCD_GenCtrl, x | crtlcd);
	}
}

static void
mach64xxdrawinit(VGAscr *scr)
{
	if(scr->io > 0x2FF){
		initengine(scr);
		scr->fill = mach64hwfill;
		scr->scroll = mach64hwscroll;
	}
/*	scr->blank = mach64blank; */
	switch(scr->id){
	default:
		break;
	case ('L'<<8)|'B':		/* 4C42: Rage 3D LTPro */
	case ('L'<<8)|'I':		/* 4C49: Rage 3D LTPro */
	case ('L'<<8)|'M':		/* 4C4D: Rage Mobility */
	case ('L'<<8)|'P':		/* 4C50: Rage 3D LTPro */
		scr->blank = mach64lcdblank;
		hwblank = 1;
		break;
	}
}

static void
ovl_configure(VGAscr *scr, Chan *c, char **field)
{
	int w, h;
	char *format;

	w = (int)strtol(field[1], nil, 0);
	h = (int)strtol(field[2], nil, 0);
	format = field[3];

	if (c != ovl_chan) 
		error(Einuse);
	if (strcmp(format, "YUYV"))
		error(Eunsupportedformat);
	
	ovl_width  = w;
	ovl_height = h;
	ovl_fib       = w * h * sizeof(ushort);

	waitforidle(scr);
	scr->mmio[mmoffset[BusCntl]] |= 0x08000000;	/* Enable regblock 1 */
	scr->mmio[mmoffset[OverlayScaleCntl]] = 
		SCALE_ZERO_EXTEND|SCALE_RED_TEMP_6500K|
		SCALE_HORZ_BLEND|SCALE_VERT_BLEND;
	scr->mmio[mmoffset[!mach64revb? Buf0Pitch: ScalerBuf0Pitch]] = w;
	scr->mmio[mmoffset[CaptureConfig]] = 
		SCALER_FRAME_READ_MODE_FULL|
		SCALER_BUF_MODE_SINGLE|
		SCALER_BUF_NEXT_0;
	scr->mmio[mmoffset[OverlayKeyCntl]] = !mach64revb?
		OVERLAY_MIX_ALWAYS_V|(OVERLAY_EXCLUSIVE_NORMAL << 28): 
		0x011;

	if (mach64type->m64_pro) {
		waitforfifo(scr, 6);

		/* set the scaler co-efficient registers */
		scr->mmio[mmoffset[ScalerColourCntl]] = 
			(0x00) | (0x10 << 8) | (0x10 << 16);
		scr->mmio[mmoffset[ScalerHCoef0]] = 
			(0x00) | (0x20 << 8);
		scr->mmio[mmoffset[ScalerHCoef1]] = 
			(0x0D) | (0x20 << 8) | (0x06 << 16) | (0x0D << 24);
		scr->mmio[mmoffset[ScalerHCoef2]] = 
			(0x0D) | (0x1C << 8) | (0x0A << 16) | (0x0D << 24);
		scr->mmio[mmoffset[ScalerHCoef3]] = 
			(0x0C) | (0x1A << 8) | (0x0E << 16) | (0x0C << 24);
		scr->mmio[mmoffset[ScalerHCoef4]] = 
			(0x0C) | (0x14 << 8) | (0x14 << 16) | (0x0C << 24);
	}
	
	waitforfifo(scr, 3);
	scr->mmio[mmoffset[VideoFormat]] = SCALE_IN_YVYU422 |
		(!mach64revb? 0xC: 0);

	if (mach64overlay == 0)
		mach64overlay = scr->storage + 64 * 64 * sizeof(uchar);
	scr->mmio[mmoffset[!mach64revb? Buf0Offset: ScalerBuf0Offset]] = 
		mach64overlay;
}

static void
ovl_enable(VGAscr *scr, Chan *c, char **field)
{
	int x, y, w, h;
	long h_inc, v_inc;

	x = (int)strtol(field[1], nil, 0);
	y = (int)strtol(field[2], nil, 0);
	w = (int)strtol(field[3], nil, 0);
	h = (int)strtol(field[4], nil, 0);

	if (x < 0 || x + w > physgscreenr.max.x ||
	     y < 0 || y + h > physgscreenr.max.y)
		error(Ebadarg);

	if (c != ovl_chan) 
		error(Einuse);
	if (scr->mmio[mmoffset[CrtcGenCntl]] & 1) {	/* double scan enable */
		y *= 2;
		h *= 2;
	}

	waitforfifo(scr, 2);
	scr->mmio[mmoffset[OverlayYX]] = 
			((x & 0xFFFF) << 16) | (y & 0xFFFF);
	scr->mmio[mmoffset[OverlayYXEnd]] = 
			(((x + w) & 0xFFFF) << 16) | ((y + h) & 0xFFFF);

	h_inc = (ovl_width << 12) / (w >> 1);  /* ??? */
	v_inc = (ovl_height << 12) / h;
	waitforfifo(scr, 2);
	scr->mmio[mmoffset[OverlayScaleInc]] = 
			((h_inc & 0xFFFF) << 16) | (v_inc & 0xFFFF);
	scr->mmio[mmoffset[ScalerHeightWidth]] = 
			((ovl_width & 0xFFFF) << 16) | (ovl_height & 0xFFFF);
	waitforidle(scr);
	scr->mmio[mmoffset[OverlayScaleCntl]] |= 
			(SCALE_ENABLE|OVERLAY_ENABLE);
}

static void
ovl_status(VGAscr *scr, Chan *, char **field)
{
	pprint("%s: %s %.4uX, VT/GT %s, PRO %s, ovlclock %d, rev B %s, refclock %ld\n",
		   scr->dev->name, field[0], mach64type->m64_id,
		   mach64type->m64_vtgt? "yes": "no",
		   mach64type->m64_pro? "yes": "no",
		   mach64type->m64_ovlclock,
		   mach64revb? "yes": "no",
		   mach64refclock);
	pprint("%s: storage @%.8luX, aperture @%8.ulX, ovl buf @%.8ulX\n",
		   scr->dev->name, scr->storage, scr->aperture,
		   mach64overlay);
}
	
static void
ovl_openctl(VGAscr *, Chan *c, char **)
{
	if (ovl_chan) 
		error(Einuse);
	ovl_chan = c;
}

static void
ovl_closectl(VGAscr *scr, Chan *c, char **)
{
	if (c != ovl_chan) return;

	waitforidle(scr);
	scr->mmio[mmoffset[OverlayScaleCntl]] &=
			~(SCALE_ENABLE|OVERLAY_ENABLE);
	ovl_chan = nil;
	ovl_width = ovl_height = ovl_fib = 0;
}

enum
{
	CMclosectl,
	CMconfigure,
	CMenable,
	CMopenctl,
	CMstatus,
};

static void (*ovl_cmds[])(VGAscr *, Chan *, char **) =
{
	[CMclosectl]	ovl_closectl,
	[CMconfigure]	ovl_configure,
	[CMenable]	ovl_enable,
	[CMopenctl]	ovl_openctl,
	[CMstatus]	ovl_status,
};

static Cmdtab mach64xxcmd[] =
{
	CMclosectl,	"closectl",	1,
	CMconfigure,	"configure",	4,
	CMenable,	"enable",	5,
	CMopenctl,	"openctl",	1,
	CMstatus,	"status",	1,
};

static void
mach64xxovlctl(VGAscr *scr, Chan *c, void *a, int n)
{
	Cmdbuf *cb;
	Cmdtab *ct;

	if(!mach64type->m64_vtgt) 
		error(Enodev);

	if(!scr->overlayinit){
		scr->overlayinit = 1;
		init_overlayclock(scr);
	}
	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}

	ct = lookupcmd(cb, mach64xxcmd, nelem(mach64xxcmd));

	ovl_cmds[ct->index](scr, c, cb->f);

	poperror();
	free(cb);
}

static int
mach64xxovlwrite(VGAscr *scr, void *a, int len, vlong offs)
{
	uchar *src;
	int _len;

	if (ovl_chan == nil) return len;	/* Acts as a /dev/null */
	
	/* Calculate the destination address */
	_len = len;
	src   = (uchar *)a;
	while (len > 0) {
		ulong _offs;
		int nb;

		_offs = (ulong)(offs % ovl_fib);
		nb     = (_offs + len > ovl_fib)? ovl_fib - _offs: len;
		memmove((uchar *)KADDR(scr->aperture + mach64overlay + _offs), 
				  src, nb);
		offs += nb;
		src  += nb;
		len  -= nb;
	}
	return _len;
}

VGAdev vgamach64xxdev = {
	"mach64xx",

	mach64xxenable,			/* enable */
	0,				/* disable */
	0,				/* page */
	mach64xxlinear,			/* linear */
	mach64xxdrawinit,	/* drawinit */
	0,
	mach64xxovlctl,	/* overlay control */
	mach64xxovlwrite,	/* write the overlay */
};

VGAcur vgamach64xxcur = {
	"mach64xxhwgc",

	mach64xxcurenable,		/* enable */
	mach64xxcurdisable,		/* disable */
	mach64xxcurload,		/* load */
	mach64xxcurmove,		/* move */

	1					/* doespanning */
};

