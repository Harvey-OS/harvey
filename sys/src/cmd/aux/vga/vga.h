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

	Pblack		= 0xFF,
	Pwhite		= 0x00,
};

enum {
	Frequency	= 14318180,	/* External Reference Clock frequency */
	VgaFreq0	= 25175000,
	VgaFreq1	= 28322000,
};

typedef struct Ctlr Ctlr;
typedef struct Vga Vga;

typedef struct Ctlr {
	char	name[NAMELEN+1];
	void	(*snarf)(Vga*, Ctlr*);
	void	(*options)(Vga*, Ctlr*);
	void	(*init)(Vga*, Ctlr*);
	void	(*load)(Vga*, Ctlr*);
	void	(*dump)(Vga*, Ctlr*);
	char*	type;

	ulong	flag;

	Ctlr*	link;
} Ctlr;

enum {					/* flag */
	Fsnarf		= 0x0001,	/* snarf done */
	Foptions	= 0x0002,	/* options done */
	Finit		= 0x0004,	/* init done */
	Fload		= 0x0008,	/* snarf done */
	Fdump		= 0x0010,	/* dump done */

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
};

typedef struct Mode {
	char	type[NAMELEN+1];	/* monitor type e.g. "vs1782" */
	char	size[NAMELEN+1];	/* size e.g. "1376x1024x8" */

	int	frequency;		/* Dot Clock (MHz) */
	int	x;			/* Horizontal Display End (Crt01), from .size[] */
	int	y;			/* Vertical Display End (Crt18), from .size[] */
	int	z;			/* depth, from .size[] */

	int	ht;			/* Horizontal Total (Crt00) */
	int	shb;			/* Start Horizontal Blank (Crt02) */
	int	ehb;			/* End Horizontal Blank (Crt03) */

	int	vt;			/* Vertical Total (Crt06) */
	int	vrs;			/* Vertical Retrace Start (Crt10) */
	int	vre;			/* Vertical Retrace End (Crt11) */

	char	hsync;
	char	vsync;
	char	interlace;
} Mode;

/*
 * The sizes of the register sets are large as many SVGA and GUI chips have extras.
 * The Crt registers are ushorts in order to keep overflow bits handy.
 * The clock elements are used for communication between the VGA, RAMDAC and clock chips;
 * they can use them however they like, it's assumed they will be used compatibly.
 */
typedef struct Vga {
	uchar	misc;
	uchar	feature;
	uchar	sequencer[256];
	ushort	crt[256];
	uchar	graphics[256];
	uchar	attribute[256];
	uchar	pixmask;
	uchar	pstatus;
	uchar	palette[Pcolours][3];

	ulong	f;			/* clock */
	ulong	d;
	ulong	i;
	ulong	n;
	ulong	p;

	ulong	vmb;			/* video memory bytes */

	Mode*	mode;

	Ctlr*	ctlr;
	Ctlr*	ramdac;
	Ctlr*	clock;
	Ctlr*	hwgc;
	Ctlr*	link;

	void*	private;
} Vga;

/* att20c49x.c */
extern Ctlr att20c491;
extern Ctlr att20c492;

/* att21c498.c */
extern uchar attdaci(uchar);
extern void attdaco(uchar, uchar);
extern Ctlr att21c498;

/* bt485.c */
extern uchar bt485i(uchar);
extern void bt485o(uchar, uchar);
extern Ctlr bt485;

/* ch9294.c */
extern Ctlr ch9294;

/* clgd542x.c */
extern Ctlr clgd542x;

/* data.c */
extern int cflag;
extern int dflag;
extern Ctlr *ctlrs[];
extern ushort dacxreg[4];

/* db.c */
extern int dbctlr(char*, Vga*);
extern Mode* dbmode(char*, char*, char*);
extern void dbdumpmode(Mode*);

/* et4000.c */
extern Ctlr et4000;

/* et4000hwgc.c */
extern Ctlr et4000hwgc;

/* ibm8514.c */
extern Ctlr ibm8514;

/* icd2061a.c */
extern Ctlr icd2061a;

/* ics2494.c */
extern Ctlr ics2494;
extern Ctlr ics2494a;

/* io.c */
extern uchar inportb(long);
extern void outportb(long, uchar);
extern ushort inportw(long);
extern void outportw(long, ushort);
extern ulong inportl(long);
extern void outportl(long, ulong);
extern char* vgactlr(char*, char*);
extern void vgactlw(char*, char*);
extern long readbios(char*, long, long);
extern void dumpbios(void);
extern void verbose(char*, ...);
extern void error(char*, ...);
extern void* alloc(ulong);
extern void printitem(char*, char*);
extern void printreg(ulong);
extern int vflag;

/* mach32.c */
extern Ctlr mach32;

/* mach64.c */
extern Ctlr mach64;

/* main.c */
extern void resyncinit(Vga*, Ctlr*, ulong, ulong);
extern void sequencer(Vga*, int);
extern void main(int, char*[]);

/* palette.c */
extern Ctlr palette;

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
extern Ctlr s3hwgc;
extern Ctlr tvp3020hwgc;

/* sc15025.c */
extern Ctlr sc15025;

/* stg1702.c */
extern Ctlr stg1702;

/* tvp3020.c */
extern uchar tvp3020i(uchar);
extern uchar tvp3020xi(uchar);
extern void tvp3020o(uchar, uchar);
extern void tvp3020xo(uchar, uchar);
extern Ctlr tvp3020;

/* tvp3025.c */
extern Ctlr tvp3025;

/* tvp3025clock.c */
extern Ctlr tvp3025clock;

/* vga.c */
extern uchar vgai(long);
extern uchar vgaxi(long, uchar);
extern void vgao(long, uchar);
extern void vgaxo(long, uchar, uchar);
extern Ctlr generic;

/* vision864.c */
extern Ctlr vision864;
