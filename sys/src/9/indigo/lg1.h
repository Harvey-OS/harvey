/*
 *	LG1 graphics board.
 */

typedef struct Configregs	Configregs;
typedef struct Rexchip		Rexchip;
typedef struct Rexregs		Rexregs;
typedef union Ul_flt		Ul_flt;

union Ul_flt
{
	float	flt;
	ulong	word;
};

typedef Ul_flt	Fixed9_11;
typedef Ul_flt	Fixed12_11;
typedef Ul_flt	Fixed12_11i;
typedef Ul_flt	Fixed12_15;
typedef Ul_flt	DDAslope;
typedef ulong	Fixed12;

struct Rexregs
{
	ulong	command;		/* instruction register	     0x0000 */
	ulong	aux1;			/* extra mode bits for GL    0x0004 */
	ulong	xstate;			/* remapping for X	     0x0008 */
	Fixed12	xstarti;		/* integer xstart	     0x000C */
	Fixed12_11 xstartf;		/* fixed point xstart	     0x0010 */
	Fixed12_15 xstart;		/* full xstart		     0x0014 */
	Fixed12_11i xendf;		/* float xend		     0x0018 */
	Fixed12	ystarti;		/* integer ystart	     0x001C */
	Fixed12_11 ystartf;		/* fixed point ystart	     0x0020 */
	Fixed12_15 ystart;		/* full ystart		     0x0024 */
	Fixed12_11i yendf;		/* float yend		     0x0028 */
	Fixed12	xsave;			/* copy of xstart	     0x002C */
	DDAslope minorslope;		/* DDA slope for line drawing0x0030 */
	ulong	xymove;			/* x, y offset from xstart   0x0034 */
	ulong	colorredi;		/* integer part of red CI reg0x0038 */
	Fixed9_11 colorredf;		/* full state of RGB red     0x003C */
	ulong	colorgreeni;		/* integer part of green reg 0x0040 */
	Fixed9_11 colorgreenf;		/* full state of RGB green   0x0044 */
	ulong	colorbluei;		/* integer part of blue reg  0x0048 */
	Fixed9_11 colorbluef;		/* full state of RGB blue    0x004C */
	DDAslope slopered;		/* DDA slope for CI or RGB   0x0050 */
	DDAslope slopegreen;		/* DDA slope for RGB shading 0x0054 */
	DDAslope slopeblue;		/* DDA slope for RGB shading 0x0058 */
	ulong	colorback;		/* background color	     0x005C */
	ulong	zpattern;		/* Z write enable mask	     0x0060 */
	ulong	lspattern;		/* line stipple pattern	     0x0064 */
	ulong	lsmode;			/* line stipple mode register0x0068 */
	ulong	aweight;		/* antialiasing weight values0x006C */
	ulong	rwaux;			/* host r/w data register    0x0070 */
	ulong	rwaux2;			/* swizzled use of rwaux1    0x0074 */
	ulong	rwmask;			/* readmask#writemask	     0x0078 */
	ulong	smask1x;		/* screenmask1 right#left    0x007C */
	ulong	smask1y;		/* screenmask1 bottom#top    0x0080 */
	Fixed12	xendi;			/* integer xend		     0x0084 */
	Fixed12 yendi;			/* integer yend		     0x0088 */
	ulong	reserved;		/* reserved                  0x008C */
};

struct Configregs
{
	ulong	smask2x;		/* screenmask2 right#left    0x4790 */
	ulong	smask2y;		/* screenmask2 bottom#top    0x4794 */
	ulong	smask3x;		/* screenmask3 right#left    0x4798 */
	ulong	smask3y;		/* screenmask3 bottom#top    0x479C */
	ulong	smask4x;		/* screenmask4 right#left    0x47A0 */
	ulong	smask4y;		/* screenmask4 bottom#top    0x47A4 */
	ulong	aux2;			/* additional mode bits	     0x47A8 */
	ulong	reserved;		/* reserved                  0x47AC */
	ulong	_pad0[10];	 	/* 0x47B0 - 0x47D4 */
	ulong	diagvram;		/* reg for VRAM write test   0x47D8 */
	ulong	diagcid;		/* reg for CID write test    0x47DC */
	ulong	_pad1; 			/* 			     0x47E0 */
	ulong	wclock;			/* NSCLOCK or REVREG	     0x47E4 */
	ulong	rwdac;			/* 8b addr/data for LUT1 I/O 0x47E8 */
	ulong	configsel;		/* select DAC or VC1 function0x47EC */
	ulong	rwvc1;			/* 8b addr/data for VC1 I/O  0x47F0 */
	ulong	togglectxt;		/* context toggle	     0x47F4 */
	ulong	configmode;		/* REX system config/monitor 0x47F8 */
	ulong	xywin;			/* screen x, y, offset	     0x47FC */
};

struct Rexchip
{
		/* page 0 */    
	Rexregs	set;				/* 0x0000 */
	char	_pad0[0x7fc-sizeof(Rexregs)];
	ulong	dummy;				/* 0x07fc */
	Rexregs	go;				/* 0x0800 */

	char	_pad1[0x4790-0x800-sizeof(Rexregs)];

		/* page 1 */
	struct{
		Configregs set;			/* 0x4790 */
		char	_pad0[0x800-sizeof(Configregs)];
		Configregs go;			/* 0x4F90 */
	}p1;
};


#define BIT(n)	(0x1 << n)

#define CHIPBUSY	BIT(0)

/*
 *  command flags
 */
#define BLOCK		BIT(3)
#define LENGTH32	BIT(4)
#define QUADMODE	BIT(5)
#define XMAJOR		BIT(6)
#define XYCONTINUE	BIT(7)
#define STOPONX		BIT(8)
#define STOPONY		BIT(9)
#define ENZPATTERN	BIT(10)
#define ENLSPATTERN	BIT(11)
#define LSADVLAST	BIT(12)
#define LSCONTINUE	BIT(13)
#define RGBMODECMD	BIT(14)
#define DITHER		BIT(15)
#define COLORCOMP	BIT(16)
#define SHADE		BIT(17)
#define INITFRAC	BIT(18)
#define LOGICSRC	BIT(19)
#define SHADECONTINUE	BIT(20)
#define COLORAUX	BIT(21)
#define LSOPAQUE	BIT(22)
#define ZOPAQUE		BIT(23)
#define ZCONTINUE	BIT(24)
#define LRQPOLY		BIT(25)

/*
 * bits in xstate register
 */

#define XSTATE_COLORAUX BIT(28)
#define XSTATE_LOGICSRC BIT(29)
#define XSTATE_LSOPAQUE BIT(30)
#define XSTATE_ZOPAQUE BIT(31)

/*
 *  rex opcodes
 */
#define REX_NOP		0x0		/* 0x7 */
#define REX_DRAW	0x1
#define REX_LSPATSAVE	0x2		/* next <== working lspat regs	*/
#define REX_LDPIXEL	0x3		/* load pixels			*/
#define REX_ANTITOP	0x4		/* anti-alias line top		*/
#define REX_ANTIBOT	0x5		/* anti-alias line bot		*/
#define REX_ANTIAUX	0x6		/* anti-alias point		*/

/*
 * logicop masks
 */
#define REX_LO_ZERO	(0x0 << 28)
#define REX_LO_AND	(0x1 << 28)
#define REX_LO_ANDR	(0x2 << 28)
#define REX_LO_SRC	(0x3 << 28)
#define REX_LO_ANDI	(0x4 << 28)
#define REX_LO_DST	(0x5 << 28)
#define REX_LO_XOR	(0x6 << 28)
#define REX_LO_OR	(0x7 << 28)
#define REX_LO_NOR	(0x8 << 28)
#define REX_LO_XNOR	(0x9 << 28)
#define REX_LO_NDST	(0xa << 28)
#define REX_LO_ORR	(0xb << 28)
#define REX_LO_NSRC	(0xc << 28)
#define REX_LO_ORI	(0xd << 28)
#define REX_LO_NAND	(0xe << 28)
#define REX_LO_ONE	(0xf << 28)

/* Auxilliary 1 flags		*/
#define FRACTION0	BIT(0)
#define FRACTION1	BIT(1)
#define DBLDST0		BIT(2)
#define DBLDST1		BIT(3)
#define DBLSRC		BIT(4)
#define DOUBLEBUF	BIT(5)
#define COLORCOMPLT	BIT(6)
#define COLORCOMPEQ	BIT(7)
#define COLORCOMPGT	BIT(8)
#define DITHERRANGE	BIT(9)

/* Auxilliary 2 flags		*/
#define SMASK0	BIT(0)
#define SMASK1	BIT(1)
#define SMASK2	BIT(2)
#define SMASK3	BIT(3)

/* smask polarity */
#define SMASK0_IN	BIT(4)
#define SMASK1_IN	BIT(5)
#define SMASK2_IN	BIT(6)
#define SMASK3_IN	BIT(7)

/* cid range masks */
#define CIDMATCH1_BITS (0xf << 8)
#define CIDMATCH2_BITS (0xf << 12)
#define CIDMATCH3_BITS (0xf << 16)
#define CIDMATCH4_BITS (0xf << 20)

/* Plane masks			*/
#define NOPLANES	(0x0 << 29)
#define PIXELPLANES	(0x1 << 29)
#define OVERLAYPLANES	(0x2 << 29)
#define CIDPLANES	(0x3 << 29)


/*
 *  VC1
 */

/* Host Command */
#define VC1_VID_TIMING	0x0
#define VC1_CURSOR_CTRL	0x0
#define VC1_DID_CTRL	0x0
#define VC1_XMAP_CTRL	0x0
#define VC1_XMAP_MODE	0x1
#define VC1_EXT_MEMORY	0x2
#define VC1_TEST_REGS	0x3
#define VC1_ADDR_LOW	0x4
#define VC1_ADDR_HIGH	0x5
#define VC1_SYS_CTRL	0x6

/* Host Interface */

#define VC1_INTERRUPT		BIT(0)  /* active low */
#define VC1_VTG			BIT(1)	/* active low */
#define VC1_VC1			BIT(2)	/* active high */
#define VC1_DID			BIT(3)	/* active high */
#define VC1_CURSOR		BIT(4)	/* active high */
#define VC1_CURSOR_DISPLAY	BIT(5)	/* active high */
#define VC1_GENSYNC		BIT(6)	/* active high */
#define VC1_VIDEO		BIT(7)	/* active high */

/* Video Timing Generator */
#define VID_EP		0x00
#define VID_LC		0x02
#define VID_SC		0x04
#define VID_TSA		0x06
#define VID_TSB		0x07
#define VID_TSC		0x08
#define VID_LP		0x09
#define VID_LS_EP	0x0b
#define VID_LR		0x0d
#define VID_FC		0x10
#define VID_ENABLE	0x14

/* Cursor Generator */
#define CUR_EP		0x20
#define CUR_XL		0x22
#define CUR_YL		0x24
#define CUR_MODE   	0x26
#define CUR_BX		0x27
#define CUR_LY		0x28
#define CUR_YC		0x2a
#define CUR_XC		0x2c
#define CUR_CC		0x2e
#define CUR_RC		0x30

#define CUR_MODE_NORMAL    0x0
#define CUR_MODE_SPECIAL   0x1
#define CUR_MODE_CROSSHAIR 0x2

#define CURSOR_XOFF 140
#define CURSOR_YOFF 39

/* Display ID Generation */
#define DID_EP		0x40
#define DID_END_EP	0x42
#define DID_HOR_DIV	0x44
#define DID_HOR_MOD	0x45
#define DID_LP		0x46
#define DID_LS_EP	0x48
#define DID_WC		0x4a
#define DID_RC		0x4c
#define DID_XL		0x4e
#define DID_XD5		0x50
#define DID_XM5		0x51
#define DID_DID		0x52

/* Control Register */
#define BLKOUT_EVEN	0x60
#define BLKOUT_ODD	0x61
#define AUXVIDEO_MAP	0x62

/* Mode Register */

/* Even XMAP */
#define MODE_REG_0_E	0x00
#define MODE_REG_1_E	0x02
#define MODE_REG_2_E	0x04
#define MODE_REG_3_E	0x06
#define MODE_REG_4_E	0x08
#define MODE_REG_5_E	0x0a
#define MODE_REG_6_E	0x0c
#define MODE_REG_7_E	0x0e
#define MODE_REG_8_E	0x10
#define MODE_REG_9_E	0x12
#define MODE_REG_A_E	0x14
#define MODE_REG_B_E	0x16
#define MODE_REG_C_E	0x18
#define MODE_REG_D_E	0x1a
#define MODE_REG_E_E	0x1c
#define MODE_REG_F_E	0x1e
#define MODE_REG_10_E	0x20
#define MODE_REG_11_E	0x22
#define MODE_REG_12_E	0x24
#define MODE_REG_13_E	0x26
#define MODE_REG_14_E	0x28
#define MODE_REG_15_E	0x2a
#define MODE_REG_16_E	0x2c
#define MODE_REG_17_E	0x2e
#define MODE_REG_18_E	0x30
#define MODE_REG_19_E	0x32
#define MODE_REG_1A_E	0x34
#define MODE_REG_1B_E	0x36
#define MODE_REG_1C_E	0x38
#define MODE_REG_1D_E	0x3a
#define MODE_REG_1E_E	0x3c
#define MODE_REG_1F_E	0x3e

/* Odd XMAP */
#define MODE_REG_0_O	0x40
#define MODE_REG_1_O	0x42
#define MODE_REG_2_O	0x44
#define MODE_REG_3_O	0x46
#define MODE_REG_4_O	0x48
#define MODE_REG_5_O	0x4a
#define MODE_REG_6_O	0x4c
#define MODE_REG_7_O	0x4e
#define MODE_REG_8_O	0x50
#define MODE_REG_9_O	0x52
#define MODE_REG_A_O	0x54
#define MODE_REG_B_O	0x56
#define MODE_REG_C_O	0x58
#define MODE_REG_D_O	0x5a
#define MODE_REG_E_O	0x5c
#define MODE_REG_F_O	0x5e
#define MODE_REG_10_O	0x60
#define MODE_REG_11_O	0x62
#define MODE_REG_12_O	0x64
#define MODE_REG_13_O	0x66
#define MODE_REG_14_O	0x68
#define MODE_REG_15_O	0x6a
#define MODE_REG_16_O	0x6c
#define MODE_REG_17_O	0x6e
#define MODE_REG_18_O	0x70
#define MODE_REG_19_O	0x72
#define MODE_REG_1A_O	0x74
#define MODE_REG_1B_O	0x76
#define MODE_REG_1C_O	0x78
#define MODE_REG_1D_O	0x7a
#define MODE_REG_1E_O	0x7c
#define MODE_REG_1F_O	0x7e

/* Test Registers */
#define CRC_SYNDROME	0x00
#define CRC_CONTROL	0x03
#define RAMTEST		0x04
#define CHIP_REVISION	0x05

/*
 * Values for RS0-RS2, host addressable registers on LUT1
 */
#define WRITE_ADDR		0
#define READ_ADDR		3
#define PALETTE_RAM		1
#define PIXEL_READ_MASK		2
#define OVERLAY_WRITE_ADDR	4
#define OVERLAY_READ_ADDR	7
#define OVERLAY			5
#define CONTROL			6

/*
 *  access modes for LUT1
 */
#define LUT_WRITE_ADDR		0
#define LUT_READ_ADDR		3
#define LUT_DATA		1
#define LUT_COMMAND		6

#define REXADDR		((Rexchip *)(KSEG1 | LIGHT_ADDR))
#define REXWAIT		while(rex->p1.set.configmode & CHIPBUSY);
