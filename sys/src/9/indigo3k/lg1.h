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
#define XYCONTINUE	BIT(7)
#define STOPONX		BIT(8)
#define STOPONY		BIT(9)
#define ENZPATTERN	BIT(10)
#define COLORAUX	BIT(21)
#define ZOPAQUE		BIT(23)

/*
 *  rex opcodes
 */
#define REX_NOP		0x0		/* 0x7 */
#define REX_DRAW	0x1

/*
 * logicop masks
 */
#define REX_LO_ZERO	(0x0 << 28)
#define REX_LO_SRC	(0x3 << 28)
#define REX_LO_NDST	(0xa << 28)
#define REX_LO_ONE	(0xf << 28)

/*
 *  VC1
 */

/* Cursor Generator */
#define CUR_EP		0x20

/*
 * Values for RS0-RS2, host addressable registers on LUT1
 */
#define WRITE_ADDR		0
#define PALETTE_RAM		1
#define PIXEL_READ_MASK		2
#define OVERLAY_WRITE_ADDR	4
#define OVERLAY			5
#define CONTROL			6

#define REXADDR		((Rexchip *)(KSEG1 | LIGHT_ADDR))
#define REXWAIT		while(rex->p1.set.configmode & CHIPBUSY);
