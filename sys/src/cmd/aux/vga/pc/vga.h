/*
 * This header file is common to both vga.c on Plan 9 and dm.c on DOS.
 */

enum vgatype {
	generic,
	tseng,
	pvga1a,
	ati,
	trident,
};

typedef union specials	specials;

typedef struct VGAmode	VGAmode;
struct VGAmode
{
	uchar	general[2];
	uchar	sequencer[5];
	uchar	crt[25];
	uchar	graphics[9];
	uchar	attribute[21];
	uchar	type;
	int	h,w,d;
	union {
		struct {
			uchar dummy[16];
		} generic;
		struct {	/*tseng lab specials*/
			uchar viden;
			uchar sr6, sr7;
			uchar ar16, ar17;
			uchar crt31, crt32;
			uchar crt33, crt34, crt35;
			uchar crt36, crt37;
		} tseng;
#define SIZEOFTSENG	13
		struct {
			uchar gr9, gra, grb, grc, grd, gre, grf;
		} pvga1a;
		struct {
			uchar ext[0xbf - 0xa0 + 1];
			uchar four6e[8];
			uchar four6f[8];
			ushort extport;	/* addr of ext registers */
		} ati;
		struct {
			uchar ext[20];
		} trident;
	} specials;
};

enum
{
	EMISCR=		0x3CC,		/* control sync polarity */
	EMISCW=		0x3C2,
	EFCW=		0x3DA,		/* feature control */
	EFCR=		0x3CA,
	GRX=		0x3CE,		/* index to graphics registers */
	GR=		0x3CF,		/* graphics registers */
	 Grms=		 0x04,		/*  read map select register */
	SRX=		0x3C4,		/* index to sequence registers */
	SR=		0x3C5,		/* sequence registers */
	 Smmask=	 0x02,		/*  map mask */
	CRX=		0x3D4,		/* index to crt registers */
	CR=		0x3D5,		/* crt registers */
	 Cvre=		 0x11,		/*  vertical retrace end */
	ARW=		0x3C0,		/* attribute registers (writing) */
	ARR=		0x3C1,		/* attribute registers (reading) */
};

extern	char *vganames[];
extern	void writeregisters(FILE *, VGAmode *);
extern	void getmode(VGAmode *, enum vgatype);
extern	void doreg(uchar in(ushort), void out(ushort, uchar), char *cp);

extern	void arout(ushort, uchar);
extern	void crout(ushort, uchar);
extern	void grout(ushort, uchar);
extern	void srout(ushort, uchar);
extern	void extout(ushort, uchar);
extern	uchar arin(ushort);
extern	uchar crin(ushort);
extern	uchar grin(ushort);
extern	uchar srin(ushort);
extern	uchar extin(ushort);

