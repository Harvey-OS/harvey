/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Generic VGA registers.
 */
enum {
	MiscW		= 0x03C2,	/* Miscellaneous Output (W) */
	MiscR		= 0x03CC,	/* Miscellaneous Output (R) */
	Status0		= 0x03C2,	/* Input status 0 (R) */
	Status1		= 0x03DA,	/* Input Status 1 (R) */
	FeatureR	= 0x03CA,	/* Feature Control (R) */
	FeatureW	= 0x03DA,	/* Feature Control (W) */

	Seqx		= 0x03C4,	/* Sequencer Index, Data at Seqx+1 */
	Crtx		= 0x03D4,	/* CRT Controller Index, Data at Crtx+1 */
	Grx		= 0x03CE,	/* Graphics Controller Index, Data at Grx+1 */
	Attrx		= 0x03C0,	/* Attribute Controller Index and Data */

	PaddrW		= 0x03C8,	/* Palette Address Register, write */
	Pdata		= 0x03C9,	/* Palette Data Register */
	Pixmask		= 0x03C6,	/* Pixel Mask Register */
	PaddrR		= 0x03C7,	/* Palette Address Register, read */
	Pstatus		= 0x03C7,	/* DAC Status (RO) */

	Pcolours	= 256,		/* Palette */
	Red		= 0,
	Green		= 1,
	Blue		= 2,

	Pblack		= 0x00,
	Pwhite		= 0xFF,
};

enum {
	RefFreq		= 14318180,	/* External Reference Clock frequency */
	VgaFreq0	= 25175000,
	VgaFreq1	= 28322000,
};

enum {
	Namelen		= 32,
};

typedef struct Ctlr Ctlr;
typedef struct Vga Vga;

typedef struct Ctlr {
	char	name[Namelen+1];
	void	(*snarf)(Vga*, Ctlr*);
	void	(*options)(Vga*, Ctlr*);
	void	(*init)(Vga*, Ctlr*);
	void	(*load)(Vga*, Ctlr*);
	void	(*dump)(Vga*, Ctlr*);
	char*	type;

	uint32_t	flag;

	Ctlr*	link;
} Ctlr;

enum {					/* flag */
	Fsnarf		= 0x00000001,	/* snarf done */
	Foptions	= 0x00000002,	/* options done */
	Finit		= 0x00000004,	/* init done */
	Fload		= 0x00000008,	/* load done */
	Fdump		= 0x00000010,	/* dump done */
	Ferror		= 0x00000020, /* error during snarf */

	Hpclk2x8	= 0x00000100,	/* have double 8-bit mode */
	Upclk2x8	= 0x00000200,	/* use double 8-bit mode */
	Henhanced	= 0x00000400,	/* have enhanced mode */
	Uenhanced	= 0x00000800,	/* use enhanced mode */
	Hpvram		= 0x00001000,	/* have parallel VRAM */
	Upvram		= 0x00002000,	/* use parallel VRAM */
	Hextsid		= 0x00004000,	/* have external SID mode */
	Uextsid		= 0x00008000,	/* use external SID mode */
	Hclk2		= 0x00010000,	/* have clock-doubler */
	Uclk2		= 0x00020000,	/* use clock-doubler */
	Hlinear		= 0x00040000,	/* have linear-address mode */
	Ulinear		= 0x00080000,	/* use linear-address mode */
	Hclkdiv		= 0x00100000,	/* have a clock-divisor */
	Uclkdiv		= 0x00200000,	/* use clock-divisor */
	Hsid32		= 0x00400000,	/* have a 32-bit (as opposed to 64-bit) SID */
};

typedef struct Attr Attr;
typedef struct Attr {
	char*	attr;
	char*	val;

	Attr*	next;
} Attr;

typedef struct Mode {
	char	type[Namelen+1];	/* monitor type e.g. "vs1782" */
	char	size[Namelen+1];	/* size e.g. "1376x1024x8" */
	char	chan[Namelen+1];	/* channel descriptor, e.g. "m8" or "r8g8b8a8" */
	char name[Namelen+1];	/* optional */

	int	frequency;		/* Dot Clock (MHz) */
	int	deffrequency;		/* Default dot clock if calculation can't be done */
	int	x;			/* Horizontal Display End (Crt01), from .size[] */
	int	y;			/* Vertical Display End (Crt18), from .size[] */
	int	z;			/* depth, from .size[] */

	int	ht;			/* Horizontal Total (Crt00) */
	int	shb;			/* Start Horizontal Blank (Crt02) */
	int	ehb;			/* End Horizontal Blank (Crt03) */

	int	shs;			/* optional Start Horizontal Sync (Crt04) */
	int	ehs;			/* optional End Horizontal Sync (Crt05) */

	int	vt;			/* Vertical Total (Crt06) */
	int	vrs;			/* Vertical Retrace Start (Crt10) */
	int	vre;			/* Vertical Retrace End (Crt11) */

	int		vbs;		/* optional Vertical Blank Start */
	int		vbe;		/* optional Vertical Blank End */
	
	uint32_t	videobw;

	char	hsync;
	char	vsync;
	char	interlace;

	Attr*	attr;
} Mode;

/*
 * The sizes of the register sets are large as many SVGA and GUI chips have extras.
 * The Crt registers are uint16_ts in order to keep overflow bits handy.
 * The clock elements are used for communication between the VGA, RAMDAC and clock chips;
 * they can use them however they like, it's assumed they will be used compatibly.
 *
 * The mode->x, mode->y coordinates are the physical size of the screen. 
 * Virtx and virty are the coordinates of the underlying memory image.
 * This can be used to implement panning around a larger screen or to cope
 * with chipsets that need the in-memory pixel line width to be a round number.
 * For example, virge.c uses this because the Savage chipset needs the pixel 
 * width to be a multiple of 16.  Also, mga2164w.c needs the pixel width
 * to be a multiple of 128.
 *
 * Vga->panning differentiates between these two uses of virtx, virty.
 *
 * (14 October 2001, rsc) Most drivers don't know the difference between
 * mode->x and virtx, a bug that should be corrected.  Vga.c, virge.c, and 
 * mga2164w.c know.  For the others, the computation that sets crt[0x13]
 * should use virtx instead of mode->x (and maybe other places change too,
 * dependent on the driver).
 */
typedef struct Vga {
	uint8_t	misc;
	uint8_t	feature;
	uint8_t	sequencer[256];
	uint16_t	crt[256];
	uint8_t	graphics[256];
	uint8_t	attribute[256];
	uint8_t	pixmask;
	uint8_t	pstatus;
	uint8_t	palette[Pcolours][3];

	uint32_t	f[2];			/* clock */
	uint32_t	d[2];
	uint32_t	i[2];
	uint32_t	m[2];
	uint32_t	n[2];
	uint32_t	p[2];
	uint32_t	q[2];
	uint32_t	r[2];

	uint32_t	vma;			/* video memory linear-address alignment */
	uint32_t	vmb;			/* video memory linear-address base */
	uint32_t	apz;			/* aperture size */
	uint32_t	vmz;			/* video memory size */

	uint32_t	membw;			/* memory bandwidth, MB/s */

	long	offset;			/* BIOS string offset */
	char*	bios;			/* matching BIOS string */
	Pcidev*	pci;			/* matching PCI device if any */

	Mode*	mode;

	uint32_t	virtx;			/* resolution of virtual screen */
	uint32_t	virty;

	int	panning;		/* pan the virtual screen */

	Ctlr*	ctlr;
	Ctlr*	ramdac;
	Ctlr*	clock;
	Ctlr*	hwgc;
	Ctlr* vesa;
	Ctlr*	link;
	int	linear;
	Attr*	attr;

	void*	private;
} Vga;

/* 3dfx.c */
extern Ctlr tdfx;
extern Ctlr tdfxhwgc;

/* ark2000pv.c */
extern Ctlr ark2000pv;
extern Ctlr ark2000pvhwgc;

/* att20c49x.c */
extern Ctlr att20c490;
extern Ctlr att20c491;
extern Ctlr att20c492;

/* att21c498.c */
extern uint8_t attdaci(uint8_t);
extern void attdaco(uint8_t, uint8_t);
extern Ctlr att21c498;

/* bt485.c */
extern uint8_t bt485i(uint8_t);
extern void bt485o(uint8_t, uint8_t);
extern Ctlr bt485;

/* ch9294.c */
extern Ctlr ch9294;

/* clgd542x.c */
extern void clgd54xxclock(Vga*, Ctlr*);
extern Ctlr clgd542x;
extern Ctlr clgd542xhwgc;

/* clgd546x.c */
extern Ctlr clgd546x;
extern Ctlr clgd546xhwgc;

/* ct65540.c */
extern Ctlr ct65540;
extern Ctlr ct65545;
extern Ctlr ct65545hwgc;

/* cyber938x.c */
extern Ctlr cyber938x;
extern Ctlr cyber938xhwgc;

/* data.c */
extern int cflag;
extern int dflag;
extern Ctlr *ctlrs[];
extern uint16_t dacxreg[4];

/* db.c */
extern char* dbattr(Attr*, char*);
extern int dbctlr(char*, Vga*);
extern Mode* dbmode(char*, char*, char*);
extern void dbdumpmode(Mode*);

/* error.c */
extern void error(char*, ...);
extern void trace(char*, ...);
extern int vflag, Vflag;

/* et4000.c */
extern Ctlr et4000;

/* et4000hwgc.c */
extern Ctlr et4000hwgc;

/* hiqvideo.c */
extern Ctlr hiqvideo;
extern Ctlr hiqvideohwgc;

/* i81x.c */
extern Ctlr i81x;
extern Ctlr i81xhwgc;

/* ibm8514.c */
extern Ctlr ibm8514;

/* icd2061a.c */
extern Ctlr icd2061a;

/* ics2494.c */
extern Ctlr ics2494;
extern Ctlr ics2494a;

/* ics534x.c */
extern Ctlr ics534x;

/* io.c */
extern uint8_t inportb(int32_t);
extern void outportb(int32_t, uint8_t);
extern uint16_t inportw(int32_t);
extern void outportw(int32_t, uint16_t);
extern uint32_t inportl(int32_t);
extern void outportl(int32_t, uint32_t);
extern char* vgactlr(char*, char*);
extern void vgactlw(char*, char*);
extern char* readbios(int32_t, int32_t);
extern void dumpbios(int32_t);
extern void error(char*, ...);
extern void* alloc(uint32_t);
extern void printitem(char*, char*);
extern void printreg(uint32_t);
extern void printflag(uint32_t);
extern void setpalette(int, int, int, int);
extern int curprintindex;

/* mach32.c */
extern Ctlr mach32;

/* mach64.c */
extern Ctlr mach64;

/* mach64xx.c */
extern Ctlr mach64xx;
extern Ctlr mach64xxhwgc;

/* main.c */
extern char* chanstr[];
extern void resyncinit(Vga*, Ctlr*, uint32_t, uint32_t);
extern void sequencer(Vga*, int);
extern void main(int, char*[]);
Biobuf stdout;

/* mga2164w.c */
extern Ctlr mga2164w;
extern Ctlr mga2164whwgc;

/* neomagic.c */
extern Ctlr neomagic;
extern Ctlr neomagichwgc;

/* nvidia.c */
extern Ctlr nvidia;
extern Ctlr nvidiahwgc;

/* radeon.c */
extern Ctlr radeon;
extern Ctlr radeonhwgc;

/* palette.c */
extern Ctlr palette;

/* pci.c */
typedef struct Pcidev Pcidev;

extern int pcicfgr8(Pcidev*, int);
extern int pcicfgr16(Pcidev*, int);
extern int pcicfgr32(Pcidev*, int);
extern void pcicfgw8(Pcidev*, int, int);
extern void pcicfgw16(Pcidev*, int, int);
extern void pcicfgw32(Pcidev*, int, int);
extern void pcihinv(Pcidev*);
extern Pcidev* pcimatch(Pcidev*, int, int);

/* rgb524.c */
extern Ctlr rgb524;

/* rgb524mn.c */
extern uint8_t (*rgb524mnxi)(Vga*, int);
extern void (*rgb524mnxo)(Vga*, int, uint8_t);
extern Ctlr rgb524mn;

/* s3801.c */
extern Ctlr s3801;
extern Ctlr s3805;

/* s3928.c */
extern Ctlr s3928;

/* s3clock.c */
extern Ctlr s3clock;

/* s3generic.c */
extern Ctlr s3generic;

/* s3hwgc.c */
extern Ctlr bt485hwgc;
extern Ctlr rgb524hwgc;
extern Ctlr s3hwgc;
extern Ctlr tvp3020hwgc;
extern Ctlr tvp3026hwgc;

/* sc15025.c */
extern Ctlr sc15025;

/* stg1702.c */
extern Ctlr stg1702;

/* t2r4.c */
extern Ctlr t2r4;
extern Ctlr t2r4hwgc;

/* trio64.c */
extern void trio64clock(Vga*, Ctlr*);
extern Ctlr trio64;

/* tvp3020.c */
extern uint8_t tvp3020i(uint8_t);
extern uint8_t tvp3020xi(uint8_t);
extern void tvp3020o(uint8_t, uint8_t);
extern void tvp3020xo(uint8_t, uint8_t);
extern Ctlr tvp3020;

/* tvp3025.c */
extern Ctlr tvp3025;

/* tvp3025clock.c */
extern Ctlr tvp3025clock;

/* tvp3026.c */
extern uint8_t tvp3026xi(uint8_t);
extern void tvp3026xo(uint8_t, uint8_t);
extern Ctlr tvp3026;

/* tvp3026clock.c */
extern Ctlr tvp3026clock;

/* vga.c */
extern uint8_t vgai(int32_t);
extern uint8_t vgaxi(int32_t, uint8_t);
extern void vgao(int32_t, uint8_t);
extern void vgaxo(int32_t, uint8_t, uint8_t);
extern Ctlr generic;

/* vesa.c */
extern Ctlr vesa;
extern Ctlr softhwgc;	/* has to go somewhere */
extern int dbvesa(Vga*);
extern Mode *dbvesamode(char*);
extern void vesatextmode(void);

/* virge.c */
extern Ctlr virge;

/* vision864.c */
extern Ctlr vision864;

/* vision964.c */
extern Ctlr vision964;

/* vision968.c */
extern Ctlr vision968;

/* vmware.c */
extern Ctlr vmware;
extern Ctlr vmwarehwgc;

/* w30c516.c */
extern Ctlr w30c516;

/* mga4xx.c */
extern Ctlr mga4xx;
extern Ctlr mga4xxhwgc;

//#pragma	varargck	argpos	error	1
//#pragma	varargck	argpos	trace	1
