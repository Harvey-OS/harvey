
/*
 * Matrox G200, G400 and G450.
 * see /sys/src/cmd/aux/vga/mga4xx.c
 */

#include 	"u.h"
#include 	"../port/lib.h"
#include 	"mem.h"
#include 	"dat.h"
#include 	"fns.h"
#include 	"io.h"
#include 	"../port/error.h"

#define	Image	IMAGE
#include 	<draw.h>
#include 	<memdraw.h>
#include 	<cursor.h>
#include	"screen.h"

enum {
	MATROX			= 0x102B,
	MGA4xx			= 0x0525,
	MGA200			= 0x0521,

	Kilo				= 1024,
	Meg				= 1024*1024,

	FCOL			= 0x1c24,
	FXRIGHT			= 0x1cac,	
	FXLEFT			= 0x1ca8,
	YDST			= 0x1c90,
	YLEN			= 0x1c5c,
 	DWGCTL			= 0x1c00,
 		DWG_TRAP		= 0x04,
 		DWG_BITBLT		= 0x08,
 		DWG_ILOAD		= 0x09,
 		DWG_LINEAR		= 0x0080,
 		DWG_SOLID		= 0x0800,
 		DWG_ARZERO		= 0x1000,
 		DWG_SGNZERO	= 0x2000,
 		DWG_SHIFTZERO	= 0x4000,
		DWG_REPLACE		= 0x000C0000,
 		DWG_REPLACE2	= (DWG_REPLACE | 0x40),
 		DWG_XOR		= 0x00060010,
 		DWG_BFCOL		= 0x04000000,
 		DWG_BMONOWF	= 0x08000000,
		DWG_TRANSC		= 0x40000000,
 	SRCORG			= 0x2cb4,
	PITCH			= 0x1c8c,
	DSTORG			= 0x2cb8,
	PLNWRT			= 0x1c1c,
	ZORG			= 0x1c0c,
	MACCESS			= 0x1c04,
	STATUS			= 0x1e14,
	FXBNDRY			= 0x1C84,
 	CXBNDRY			= 0x1C80,
	YTOP			= 0x1C98,
	YBOT			= 0x1C9C,
	YDSTLEN			= 0x1C88,
	AR0				= 0x1C60,
	AR1				= 0x1C64,
	AR2				= 0x1C68,
	AR3				= 0x1C6C,
	AR4				= 0x1C70,
	AR5				= 0x1C74,
	SGN				= 0x1C58,
		SGN_SCANLEFT = 		1,
		SGN_SCANRIGHT = 		0,
		SGN_SDY_POSITIVE = 	0,
		SGN_SDY_NEGATIVE = 	4,

	GO				= 0x0100,
 	FIFOSTATUS		= 0x1E10,
	CACHEFLUSH		= 0x1FFF,

	CRTCEXTIDX		= 0x1FDE,		/* CRTC Extension Index */
	CRTCEXTDATA		= 0x1FDF,		/* CRTC Extension Data */

	FILL_OPERAND		= 0x800c7804,
};

static Pcidev*
mgapcimatch(void)
{
	Pcidev*	p;
	
	p = pcimatch(nil, MATROX, MGA4xx);
	if (p == nil)
		p = pcimatch(nil, MATROX, MGA200);
	return p;
}

static ulong
mga4xxlinear(VGAscr* scr, int* size, int* align)
{
	ulong 	aperture, oaperture;
	int 		oapsize, wasupamem;
	Pcidev *	p;

	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	if(p = mgapcimatch()){
		aperture = p->mem[0].bar & ~0x0F;
		if(p->did == MGA4xx)
			*size = 32*Meg;
		else
			*size = 8*Meg;
	}
	else
		aperture = 0;

	if(wasupamem) {
		if(oaperture == aperture)
			return oaperture;
		upafree(oaperture, oapsize);
	}
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0)) {
			aperture = oaperture;
			scr->isupamem = 1;
		}
		else
			scr->isupamem = 0;
	}
	else
		scr->isupamem = 1;

	return aperture;
}

static void
mgawrite8(VGAscr* scr, int index, uchar val)
{
	((uchar*)scr->io)[index] = val;
}

static uchar
mgaread8(VGAscr* scr, int index)
{
	return ((uchar*)scr->io)[index];
}

static uchar
crtcextset(VGAscr* scr, int index, uchar set, uchar clr)
{
	uchar	tmp;

	mgawrite8(scr, CRTCEXTIDX, index);
	tmp = mgaread8(scr, CRTCEXTDATA);
	mgawrite8(scr, CRTCEXTIDX, index);
	mgawrite8(scr, CRTCEXTDATA, (tmp & ~clr) | set);

	return tmp;
}

static void
mga4xxenable(VGAscr* scr)
{
	Pcidev *	pci;
	int 		size, align;
	ulong 	aperture;
	int 		i, n, k;
	uchar *	p;
	uchar	x[16];
	uchar	crtcext3;

	/*
	 * Only once, can't be disabled for now.
	 * scr->io holds the virtual address of
	 * the MMIO registers.
	 */
	if(scr->io)
		return;

	pci = mgapcimatch();
	if(pci == nil)
		return;

	scr->io = upamalloc(pci->mem[1].bar & ~0x0F, 16*1024, 0);
	if(scr->io == 0)
		return;

	addvgaseg("mga4xxmmio", scr->io, pci->mem[1].size);

	scr->io = (ulong)KADDR(scr->io);

	/* need to map frame buffer here too, so vga can find memory size */
	size = 8*Meg;
	align = 0;
	aperture = mga4xxlinear(scr, &size, &align);
	if(aperture) {
		scr->aperture = aperture;
		addvgaseg("mga4xxscreen", aperture, size);

		/* Find out how much memory is here, some multiple of 2 Meg */

		/* First Set MGA Mode ... */
		crtcext3 = crtcextset(scr, 3, 0x80, 0x00);

		p = (uchar*)aperture;
		n = (size / Meg) / 2;
		for (i = 0; i < n; i++) {
			k = (2*i+1)*Meg;
			p[k] = 0;
			p[k] = i+1;
			*((uchar*)(scr->io + CACHEFLUSH)) = 0;
			x[i] = p[k];
 		}
		for(i = 1; i < n; i++)
			if(x[i] != i+1)
				break;
  		scr->apsize = 2*i*Meg;

		crtcextset(scr, 3, crtcext3, 0xff);
	}
}

enum {
	Index		= 0x00,		/* Index */
	Data			= 0x0A,		/* Data */

	Cxlsb		= 0x0C,		/* Cursor X LSB */
	Cxmsb		= 0x0D,		/* Cursor X MSB */
	Cylsb		= 0x0E,		/* Cursor Y LSB */
	Cymsb		= 0x0F,		/* Cursor Y MSB */

	Icuradrl		= 0x04,		/* Cursor Base Address Low */	
	Icuradrh		= 0x05,		/* Cursor Base Address High */
	Icctl			= 0x06,		/* Indirect Cursor Control */
};

static void
dac4xxdisable(VGAscr* scr)
{
	uchar * 	dac4xx;
	
	if(scr->io == 0)
		return;

	dac4xx = KADDR(scr->io+0x3C00);
	
	*(dac4xx+Index) = Icctl;
	*(dac4xx+Data) = 0x00;
}

static void
dac4xxload(VGAscr* scr, Cursor* curs)
{
	int 		y;
	uchar *	p;
	uchar * 	dac4xx;

	if(scr->io == 0)
		return;

	dac4xx = KADDR(scr->io+0x3C00);
	
	dac4xxdisable(scr);

	p = KADDR(scr->storage);
	for(y = 0; y < 64; y++){
		*p++ = 0; *p++ = 0; *p++ = 0;
		*p++ = 0; *p++ = 0; *p++ = 0;
		if(y <16){
			*p++ = curs->set[1+y*2]|curs->clr[1+2*y];
			*p++ = curs->set[y*2]|curs->clr[2*y];
		} else{
			*p++ = 0; *p++ = 0;
		}

		*p++ = 0; *p++ = 0; *p++ = 0;
		*p++ = 0; *p++ = 0; *p++ = 0;
		if(y <16){
			*p++ = curs->set[1+y*2];
			*p++ = curs->set[y*2];
		} else{
			*p++ = 0; *p++ = 0;
		}
	}
	scr->offset.x = 64 + curs->offset.x;
	scr->offset.y = 64 + curs->offset.y;

	*(dac4xx+Index) = Icctl;
	*(dac4xx+Data) = 0x03;
}

static int
dac4xxmove(VGAscr* scr, Point p)
{
	int 		x, y;
	uchar *	dac4xx;

	if(scr->io == 0)
		return 1;

	dac4xx = KADDR(scr->io + 0x3C00);

	x = p.x + scr->offset.x;
	y = p.y + scr->offset.y;

	*(dac4xx+Cxlsb) = x & 0xFF;
	*(dac4xx+Cxmsb) = (x>>8) & 0x0F;

	*(dac4xx+Cylsb) = y & 0xFF;
	*(dac4xx+Cymsb) = (y>>8) & 0x0F;

	return 0;
}

static void
dac4xxenable(VGAscr* scr)
{
	uchar *	dac4xx;
	ulong	storage;
	
	if(scr->io == 0)
		return;
	dac4xx = KADDR(scr->io+0x3C00);

	dac4xxdisable(scr);

	storage = (scr->apsize - 4096) & ~0x3ff;

	*(dac4xx+Index) = Icuradrl;
	*(dac4xx+Data) = 0xff & (storage >> 10);
	*(dac4xx+Index) = Icuradrh;
	*(dac4xx+Data) = 0xff & (storage >> 18);		

	scr->storage = (ulong) KADDR((ulong)scr->aperture + (ulong)storage);

	/* Show X11-Like Cursor */
	*(dac4xx+Index) = Icctl;
	*(dac4xx+Data) = 0x03;

	/* Cursor Color 0 : White */
	*(dac4xx+Index) = 0x08;
	*(dac4xx+Data)  = 0xff;
	*(dac4xx+Index) = 0x09;
	*(dac4xx+Data)  = 0xff;
	*(dac4xx+Index) = 0x0a;
	*(dac4xx+Data)  = 0xff;

	/* Cursor Color 1 : Black */
	*(dac4xx+Index) = 0x0c;
	*(dac4xx+Data)  = 0x00;
	*(dac4xx+Index) = 0x0d;
	*(dac4xx+Data)  = 0x00;
	*(dac4xx+Index) = 0x0e;
	*(dac4xx+Data)  = 0x00;

	/* Cursor Color 2 : Red */
	*(dac4xx+Index) = 0x10;
	*(dac4xx+Data)  = 0xff;
	*(dac4xx+Index) = 0x11;
	*(dac4xx+Data)  = 0x00;
	*(dac4xx+Index) = 0x12;
	*(dac4xx+Data)  = 0x00;

	/*
	 * Load, locate and enable the
	 * 64x64 cursor in X11 mode.
	 */
	dac4xxload(scr, &arrow);
	dac4xxmove(scr, ZP);
}

static void
mga4xxblank(VGAscr* scr, int blank)
{
	char * 	cp;
	uchar * 	mga;
	uchar 	seq1, crtcext1;
	
	/* blank = 0 -> turn screen on */
	/* blank = 1 -> turn screen off */

	if(scr->io == 0)
		return;
	mga = KADDR(scr->io);	

	if (blank == 0) {
		seq1 = 0x00;
		crtcext1 = 0x00;
	} else {
		seq1 = 0x20;
		crtcext1 = 0x10;			/* Default value ... : standby */
		cp = getconf("*dpms");
		if (cp) {
			if (cistrcmp(cp, "standby") == 0) {
				crtcext1 = 0x10;
			} else if (cistrcmp(cp, "suspend") == 0) {
				crtcext1 = 0x20;
			} else if (cistrcmp(cp, "off") == 0) {
				crtcext1 = 0x30;
			}
		}
	}

	*(mga + 0x1fc4) = 1;
	seq1 |= *(mga + 0x1fc5) & ~0x20;
	*(mga + 0x1fc5) = seq1;

	*(mga + 0x1fde) = 1;
	crtcext1 |= *(mga + 0x1fdf) & ~0x30;
	*(mga + 0x1fdf) = crtcext1;
}

static void
mgawrite32(uchar * mga, ulong reg, ulong val)
{
	ulong *	l;

	l = (ulong *)(&mga[reg]);
	l[0] = val;
}

static ulong
mgaread32(uchar * mga, ulong reg)
{
	return *((ulong *)(&mga[reg]));
}

static int
mga4xxfill(VGAscr* scr, Rectangle r, ulong color)
{
	uchar * 		mga;
 
	/* Constant Shaded Trapezoids / Rectangle Fills */
	if(scr->io == 0)
		return 0;
	mga = KADDR(scr->io);

	mgawrite32(mga, DWGCTL, 0);
	mgawrite32(mga, FCOL, color);
	mgawrite32(mga, FXRIGHT, r.max.x);
	mgawrite32(mga, FXLEFT, r.min.x);
	mgawrite32(mga, YDST, r.min.y);
	mgawrite32(mga, YLEN, Dy(r));
	mgawrite32(mga, DWGCTL + GO, FILL_OPERAND);

	while (mgaread32(mga, STATUS) & 0x00010000)
		;
	return 1;
}

#define mga_fifo(n)	do {} while ((mgaread32(mga, FIFOSTATUS) & 0xFF) < (n))

static int
mga4xxscroll(VGAscr* scr, Rectangle r_dst, Rectangle r_src)
{
	uchar * 	mga;
	ulong	pitch, y;
 	ulong 	width, height, start, end, scandir;
	int 		ydir;
 
	/* Two-operand Bitblts */
	if(scr->io == 0)
		return 0;

	mga = KADDR(scr->io);

	pitch = Dx(scr->gscreen->r);

	mgawrite32(mga, DWGCTL, 0);

 	scandir 	= 0;

	height 	= abs(Dy(r_src));
	width 	= abs(Dx(r_src));

	assert(height == abs(Dy(r_dst)));
	assert(width == abs(Dx(r_dst)));

	if ((r_src.min.y == r_dst.min.y) && (r_src.min.x == r_dst.min.x))
	{
		if (0) 
			print("move x,y to x,y !\n");
		return 1;
	}

	ydir = 1;
	if (r_dst.min.y > r_src.min.y)
	{
		if (0) 
			print("ydir = -1\n");
		ydir = -1;
		scandir |= 4;	// Blit UP
	}

	if (r_dst.min.x > r_src.min.x)
	{
		if (0) 
			print("xdir = -1\n");
		scandir |= 1;	// Blit Left
	}

	mga_fifo(4);
	if (scandir)
	{
 		mgawrite32(mga, DWGCTL, DWG_BITBLT | DWG_SHIFTZERO | 
			DWG_SGNZERO | DWG_BFCOL | DWG_REPLACE);
		mgawrite32(mga, SGN, scandir);
	} else
	{
		mgawrite32(mga, DWGCTL, DWG_BITBLT | DWG_SHIFTZERO | 
			DWG_BFCOL | DWG_REPLACE);
 	}
	mgawrite32(mga, AR5, ydir * pitch);

	width--;
	start = end = r_src.min.x + (r_src.min.y * pitch);
	if ((scandir & 1) == 1)
	{
		start += width;
	} else
	{
		end += width;
	}

	y = r_dst.min.y;
	if ((scandir & 4) == 4)
	{
		start += (height - 1) * pitch;
		end += (height - 1) * pitch;
		y += (height - 1);
	}

	mga_fifo(4);
	mgawrite32(mga, AR0, end);
	mgawrite32(mga, AR3, start);
	mgawrite32(mga, FXBNDRY, ((r_dst.min.x+width)<<16) | r_dst.min.x);
	mgawrite32(mga, YDSTLEN + GO, (y << 16) | height);
 
	if (1)
	{
		while (mgaread32(mga, STATUS) & 0x00010000)
			;
	}

	return 1;
}

static void
mga4xxdrawinit(VGAscr* scr)
{
	uchar * 	mga;
 	Pcidev*	p;

	p = pcimatch(nil, MATROX, MGA4xx);
	if (p == nil)
		return ;

	if(scr->io == 0)
		return;
	mga = KADDR(scr->io);

	mgawrite32(mga, SRCORG, 0);
	mgawrite32(mga, DSTORG, 0);
	mgawrite32(mga, ZORG, 0);
	mgawrite32(mga, PLNWRT, ~0);
	mgawrite32(mga, FCOL, 0xffff0000);
	mgawrite32(mga, CXBNDRY, 0xFFFF0000);
	mgawrite32(mga, YTOP, 0);
	mgawrite32(mga, YBOT, 0x01FFFFFF);
	mgawrite32(mga, PITCH, Dx(scr->gscreen->r) & ((1 << 13) - 1));
	switch(scr->gscreen->depth) {
	case 8:
		mgawrite32(mga, MACCESS, 0);
		break;
	case 32:
		mgawrite32(mga, MACCESS, 2);
		break;
	default:
		return;		/* depth not supported ! */
	}
	scr->fill = mga4xxfill;
	scr->scroll = mga4xxscroll;
	scr->blank = mga4xxblank;
}

VGAdev vgamga4xxdev = {
	"mga4xx",

	mga4xxenable,		/* enable */
	0,					/* disable */
	0,					/* page */
	mga4xxlinear,			/* linear */
	mga4xxdrawinit,
};

VGAcur vgamga4xxcur = {
	"mga4xxhwgc",
	dac4xxenable,
	dac4xxdisable,
	dac4xxload,
	dac4xxmove,
};
