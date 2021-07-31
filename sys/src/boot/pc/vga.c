#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

#define	MINX	8

extern	GFont	defont0;
GFont		*defont;

struct{
	Point	pos;
	int	bwid;
}out;

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

typedef struct VGAmode	VGAmode;
struct VGAmode
{
	uchar	general[4];
	uchar	sequencer[5];
	uchar	crt[0x19];
	uchar	graphics[9];
	uchar	attribute[0x15];
};

/*
 *  640x480 display, 16 bit color
 */
VGAmode mode12 = 
{
	/* general */
	0xe3, 0x00, 0x70, 0x04,
	/* sequence */
	0x03, 0x01, 0x0f, 0x00, 0x06,
	/* crt */
	0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59,
	0xea, 0x0c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3,
	0xff,
	/* graphics */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0f,
	0xff,
	/* attribute */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x01, 0x00, 0x0f, 0x00, 0x00,
};

void
genout(int reg, int val)
{
	if(reg == 0)
		outb(EMISCW, val);
	else if (reg == 1)
		outb(EFCW, val);
}
void
srout(int reg, int val)
{
	outb(SRX, reg);
	outb(SR, val);
}
void
grout(int reg, int val)
{
	outb(GRX, reg);
	outb(GR, val);
}
void
arout(int reg, int val)
{
	inb(0x3DA);
	outb(ARX, reg | 0x20);
	outb(AR, val);
}
void
crout(int reg, int val)
{
	outb(CRX, reg);
	outb(CR, val);
}

/*
 *  m is a bit mask of planes to be affected by CPU writes
 */
vgawrmask(int m)
{
	srout(Smmask, m&0xf);
}

/*
 *  p is the plane that will respond to CPU reads
 */
vgardplane(int p)
{
	grout(Grms, p&3);
}

vgadump(void)
{
	print("misc is 0x%ux fc is 0x%ux\n", inb(EMISCR), inb(EFCR));
}

void
setmode(VGAmode *v)
{
	int i;

	for(i = 0; i < sizeof(v->general); i++)
		genout(i, v->general[i]);

	for(i = 0; i < sizeof(v->sequencer); i++)
		srout(i, v->sequencer[i]);

	crout(Cvre, 0);	/* allow writes to CRT registers 0-7 */
	for(i = 0; i < sizeof(v->crt); i++)
		crout(i, v->crt[i]);

	for(i = 0; i < sizeof(v->graphics); i++)
		grout(i, v->graphics[i]);

	for(i = 0; i < sizeof(v->attribute); i++)
		arout(i, v->attribute[i]);
}

void
screeninit(void)
{
	int i, j, k;
	int c;
	ulong *l;

	setmode(&mode12);

	/*
	 *  swizzle the font longs.
	 *  we do it here since the font is initialized with big
	 *  endian longs.
	 */
	defont = &defont0;
	l = defont->bits->base;
	for(i = defont->bits->width*Dy(defont->bits->r); i > 0; i--, l++)
		*l = (*l<<24) | ((*l>>8)&0x0000ff00) | ((*l<<8)&0x00ff0000) | (*l>>24);

	gbitblt(&gscreen, Pt(0, 0), &gscreen, gscreen.r, 0);
	out.pos.x = MINX;
	out.pos.y = 0;
	out.bwid = defont0.info[' '].width;
}

void
screenputc(int c)
{
	char buf[2];
	int nx;

	if(c == '\n'){
		out.pos.x = MINX;
		out.pos.y += defont0.height;
		if(out.pos.y > gscreen.r.max.y-defont0.height)
			out.pos.y = gscreen.r.min.y;
		gbitblt(&gscreen, Pt(0, out.pos.y), &gscreen,
		    Rect(0, out.pos.y, gscreen.r.max.x, out.pos.y+2*defont0.height), 0);
	}else if(c == '\t'){
		out.pos.x += (8-((out.pos.x-MINX)/out.bwid&7))*out.bwid;
		if(out.pos.x >= gscreen.r.max.x)
			screenputc('\n');
	}else if(c == '\b'){
		if(out.pos.x >= out.bwid+MINX){
			out.pos.x -= out.bwid;
			screenputc(' ');
			out.pos.x -= out.bwid;
		}
	}else{
		if(out.pos.x >= gscreen.r.max.x-out.bwid)
			screenputc('\n');
		buf[0] = c&0x7F;
		buf[1] = 0;
		out.pos = gstring(&gscreen, out.pos, defont, buf, S);
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
	return 1;	/* bits per pixel */
}

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

int
hwcursset(uchar *s, uchar *c, int ox, int oy)
{
	return 0;
}

int
hwcursmove(int x, int y)
{
	return 0;
}

vgaset(char *cmd)
{
	int set;
	int reg;
	int val;

	set = *cmd++;
	cmd++;
	reg = strtoul(cmd, &cmd, 0);
	cmd++;
	val = strtoul(cmd, &cmd, 0);
	switch(set){
	case 'a':
		arout(reg, val);
		break;
	case 'g':
		grout(reg, val);
		break;
	case 'c':
		crout(reg, val);
		break;
	case 's':
		srout(reg, val);
		break;
	}
}
