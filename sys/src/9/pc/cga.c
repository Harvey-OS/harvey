#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#define	WIDTH	160
#define	HEIGHT	24
#define SCREEN	((char *)(0xB8000|KZERO))
int pos;

void
screeninit(void)
{
	void vgadump(void);

	memset(SCREEN+pos, 0, WIDTH*HEIGHT-pos);
	vgadump();
}

void
screenputc(int c)
{
	int i;
	static int color;

	if(c == '\n'){
		pos = pos/WIDTH;
		pos = (pos+1)*WIDTH;
	} else if(c == '\t'){
		i = 8 - ((pos/2)&7);
		while(i-->0)
			screenputc(' ');
	} else if(c == '\b'){
		if(pos >= 2)
			pos -= 2;
		screenputc(' ');
		pos -= 2;
	} else {
		SCREEN[pos++] = c;
		SCREEN[pos++] = 0x1d;
	}
	if(pos >= WIDTH*HEIGHT){
		memmove(SCREEN, &SCREEN[WIDTH], WIDTH*(HEIGHT-1));
		memset(&SCREEN[WIDTH*(HEIGHT-1)], 0, WIDTH);
		pos = WIDTH*(HEIGHT-1);
	}
}

void
screenputs(char *s, int n)
{
	while(n-- > 0)
		screenputc(*s++);
}

int
screenbits(void)
{
	return 4;
}


#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

/*
 *  screen dimensions
 */
#define MAXX	640
#define MAXY	480

#define SCREENMEM	(0xA0000 | KZERO)
GBitmap	gscreen =
{
	(ulong*)SCREENMEM,
	0,
	640/32,
	0,
	0, 0, MAXX, MAXY,
	0
};

typedef struct VGAmode	VGAmode;
struct VGAmode
{
	uchar	general[4];
	uchar	sequencer[5];
	uchar	crt[0x19];
	uchar	graphics[9];
	uchar	attribute[0x15];
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
	ARX=		0x3C0,		/* index to attribute registers */
	AR=		0x3C1,		/* attribute registers */
};

uchar
genin(int reg)
{
	if(reg == 0)
		return inb(EMISCR);
	else if (reg == 1)
		return inb(EFCR);
}
uchar
srin(int reg)
{
	outb(SRX, reg);
	return inb(SR);
}
uchar
grin(int reg)
{
	outb(GRX, reg);
	return inb(GR);
}
uchar
arin(int reg)
{
	inb(0x3DA);
	outb(ARX, reg | 0x20);
	return inb(AR);
}
uchar
crin(int reg)
{
	outb(CRX, reg);
	return inb(CR);
}

void
vgadump(void)
{
	int i;
	VGAmode *v;

	print("GEN ");
	for(i = 0; i < sizeof(v->general); i++)
		print("%d-%ux ", i, genin(i));
	print("\nSR ");
	for(i = 0; i < sizeof(v->sequencer); i++)
		print("%d-%ux ", i, srin(i));
	print("\nCR ");
	for(i = 0; i < sizeof(v->crt); i++)
		print("%d-%ux ", i, crin(i));
	print("\nGR ");
	for(i = 0; i < sizeof(v->graphics); i++)
		print("%d-%ux ", i, grin(i));
	print("\nAR ");
	for(i = 0; i < sizeof(v->attribute); i++)
		print("%d-%ux ", i, arin(i));
	print("\n ");
}

void
bigcursor(void)
{}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	ulong ans;

	/*
	 * The safari monochrome says 0 is black (zero intensity)
	 */
	if(p == 0)
		ans = 0;
	else
		ans = ~0;
	*pr = *pg = *pb = ans;
}


int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	return 0;	/* can't change mono screen colormap */
}

void
mouseclock(void)
{
	mouseupdate(1);
}
