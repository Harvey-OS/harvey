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
	PCIVMWARE	= 0x15AD,	/* PCI VID */

	VMWARE1		= 0x0710,	/* PCI DID */
	VMWARE2		= 0x0405,
};

enum {
	Rid = 0,
	Renable,
	Rwidth,
	Rheight,
	Rmaxwidth,

	Rmaxheight,
	Rdepth,
	Rbpp,
	Rpseudocolor,
	Rrmask,

	Rgmask,
	Rbmask,
	Rbpl,
	Rfbstart,
	Rfboffset,

	Rfbmaxsize,
	Rfbsize,
	Rcap,
	Rmemstart,
	Rmemsize,

	Rconfigdone,
	Rsync,
	Rbusy,
	Rguestid,
	Rcursorid,

	Rcursorx,
	Rcursory,
	Rcursoron,
	Nreg,

	Crectfill = 1<<0,
	Crectcopy = 1<<1,
	Crectpatfill = 1<<2,
	Coffscreen = 1<<3,
	Crasterop = 1<<4,
	Ccursor = 1<<5,
	Ccursorbypass = 1<<6,
	Ccursorbypass2 = 1<<7,
	C8bitemulation = 1<<8,
	Calphacursor = 1<<9,

	FifoMin = 0,
	FifoMax = 1,
	FifoNextCmd = 2,
	FifoStop = 3,
	FifoUser = 4,

	Xupdate = 1,
	Xrectfill = 2,
	Xrectcopy = 3,
	Xdefinebitmap = 4,
	Xdefinebitmapscanline = 5,
	Xdefinepixmap = 6,
	Xdefinepixmapscanline = 7,
	Xrectbitmapfill = 8,
	Xrectpixmapfill = 9,
	Xrectbitmapcopy = 10,
	Xrectpixmapcopy = 11,
	Xfreeobject = 12,
	Xrectropfill = 13,
	Xrectropcopy = 14,
	Xrectropbitmapfill = 15,
	Xrectroppixmapfill = 16,
	Xrectropbitmapcopy = 17,
	Xrectroppixmapcopy = 18,
	Xdefinecursor = 19,
	Xdisplaycursor = 20,
	Xmovecursor = 21,
	Xdefinealphacursor = 22,
	Xcmdmax = 23,

	CursorOnHide = 0,
	CursorOnShow = 1,
	CursorOnRemoveFromFb = 2,
	CursorOnRestoreToFb = 3,

	Rpalette = 1024,
};

typedef struct Vmware	Vmware;
struct Vmware {
	ulong	fb;

	ulong	ra;
	ulong	rd;

	ulong	r[Nreg];
	ulong	*mmio;
	ulong	mmiosize;

	char	chan[32];
	int	depth;
};

Vmware xvm;
Vmware *vm=&xvm;

static ulong
vmrd(Vmware *vm, int i)
{
	outl(vm->ra, i);
	return inl(vm->rd);
}

static void
vmwr(Vmware *vm, int i, ulong v)
{
	outl(vm->ra, i);
	outl(vm->rd, v);
}

static void
vmwait(Vmware *vm)
{
	vmwr(vm, Rsync, 1);
	while(vmrd(vm, Rbusy))
		;
}

static ulong
vmwarelinear(VGAscr* scr, int* size, int* align)
{
	char err[64];
	ulong aperture, oaperture;
	int osize, oapsize, wasupamem;
	Pcidev *p;

	osize = *size;
	oaperture = scr->aperture;
	oapsize = scr->apsize;
	wasupamem = scr->isupamem;

	p = pcimatch(nil, PCIVMWARE, 0);
	if(p == nil)
		error("no vmware card found");

	switch(p->did){
	default:
		snprint(err, sizeof err, "unknown vmware id %.4ux", p->did);
		error(err);
		
	case VMWARE1:
		vm->ra = 0x4560;
		vm->rd = 0x4560+4;
		break;

	case VMWARE2:
		vm->ra = p->mem[0].bar&~3;
		vm->rd = vm->ra + 1;
	}

	aperture = (ulong)(vmrd(vm, Rfbstart));
	*size = vmrd(vm, Rfbsize);

	if(wasupamem)
		upafree(oaperture, oapsize);
	scr->isupamem = 0;

	aperture = upamalloc(aperture, *size, *align);
	if(aperture == 0){
		if(wasupamem && upamalloc(oaperture, oapsize, 0))
			scr->isupamem = 1;
	}else
		scr->isupamem = 1;

	if(oaperture && aperture != oaperture)
		print("warning (BUG): redefinition of aperture does not change vmwarescreen segment\n");
	addvgaseg("vmwarescreen", aperture, osize);

	return aperture;
}

static void
vmfifowr(Vmware *vm, ulong v)
{
	ulong *mm;

	mm = vm->mmio;
	if(mm == nil){
		iprint("!");
		return;
	}

	if(mm[FifoNextCmd]+sizeof(ulong) == mm[FifoStop]
	|| (mm[FifoNextCmd]+sizeof(ulong) == mm[FifoMax]
	    && mm[FifoStop] == mm[FifoMin]))
		vmwait(vm);

	mm[mm[FifoNextCmd]/sizeof(ulong)] = v;

	/* must do this way so mm[FifoNextCmd] is never mm[FifoMax] */
	v = mm[FifoNextCmd] + sizeof(ulong);
	if(v == mm[FifoMax])
		v = mm[FifoMin];
	mm[FifoNextCmd] = v;
}

static void
vmwareflush(VGAscr*, Rectangle r)
{
	if(vm->mmio == nil)
		return;

	vmfifowr(vm, Xupdate);
	vmfifowr(vm, r.min.x);
	vmfifowr(vm, r.min.y);
	vmfifowr(vm, r.max.x-r.min.x);
	vmfifowr(vm, r.max.y-r.min.y);
	vmwait(vm);
}

static void
vmwareload(VGAscr*, Cursor *c)
{
	int i;
	ulong clr, set;
	ulong and[16];
	ulong xor[16];

	if(vm->mmio == nil)
		return;
	vmfifowr(vm, Xdefinecursor);
	vmfifowr(vm, 1);	/* cursor id */
	vmfifowr(vm, -c->offset.x);
	vmfifowr(vm, -c->offset.y);

	vmfifowr(vm, 16);	/* width */
	vmfifowr(vm, 16);	/* height */
	vmfifowr(vm, 1);	/* depth for and mask */
	vmfifowr(vm, 1);	/* depth for xor mask */

	for(i=0; i<16; i++){
		clr = (c->clr[i*2+1]<<8) | c->clr[i*2];
		set = (c->set[i*2+1]<<8) | c->set[i*2];
		and[i] = ~(clr|set);	/* clr and set pixels => black */
		xor[i] = clr&~set;		/* clr pixels => white */
	}
	for(i=0; i<16; i++)
		vmfifowr(vm, and[i]);
	for(i=0; i<16; i++)
		vmfifowr(vm, xor[i]);

	vmwait(vm);
}

static int
vmwaremove(VGAscr*, Point p)
{
	vmwr(vm, Rcursorid, 1);
	vmwr(vm, Rcursorx, p.x);
	vmwr(vm, Rcursory, p.y);
	vmwr(vm, Rcursoron, CursorOnShow);
	return 0;
}

static void
vmwaredisable(VGAscr*)
{
	vmwr(vm, Rcursorid, 1);
	vmwr(vm, Rcursoron, CursorOnHide);
}

static void
vmwareenable(VGAscr*)
{
	vmwr(vm, Rcursorid, 1);
	vmwr(vm, Rcursoron, CursorOnShow);
}

static void
vmwareblank(int)
{
}

static int
vmwarescroll(VGAscr*, Rectangle r, Rectangle sr)
{
	if(vm->mmio == nil)
		return 0;
	vmfifowr(vm, Xrectcopy);
	vmfifowr(vm, sr.min.x);
	vmfifowr(vm, sr.min.y);
	vmfifowr(vm, r.min.x);
	vmfifowr(vm, r.min.y);
	vmfifowr(vm, Dx(r));
	vmfifowr(vm, Dy(r));
	vmwait(vm);
	return 1;
}

static int
vmwarefill(VGAscr*, Rectangle r, ulong sval)
{
	if(vm->mmio == nil)
		return 0;
	vmfifowr(vm, Xrectfill);
	vmfifowr(vm, sval);
	vmfifowr(vm, r.min.x);
	vmfifowr(vm, r.min.y);
	vmfifowr(vm, r.max.x-r.min.x);
	vmfifowr(vm, r.max.y-r.min.y);
	vmwait(vm);
	return 1;
}

static void
vmwaredrawinit(VGAscr *scr)
{
	ulong offset;
	ulong mmiobase, mmiosize;

	if(scr->mmio==nil){
		mmiobase = vmrd(vm, Rmemstart);
		if(mmiobase == 0)
			return;
		mmiosize = vmrd(vm, Rmemsize);
		scr->mmio = KADDR(upamalloc(mmiobase, mmiosize, 0));
		vm->mmio = scr->mmio;
		vm->mmiosize = mmiosize;
		if(scr->mmio == nil)
			return;
		addvgaseg("vmwaremmio", mmiobase, mmiosize);
	}

	scr->mmio[FifoMin] = 4*sizeof(ulong);
	scr->mmio[FifoMax] = vm->mmiosize;
	scr->mmio[FifoNextCmd] = 4*sizeof(ulong);
	scr->mmio[FifoStop] = 4*sizeof(ulong);
	vmwr(vm, Rconfigdone, 1);

	scr->scroll = vmwarescroll;
	scr->fill = vmwarefill;

	offset = vmrd(vm, Rfboffset);
	scr->gscreendata->bdata += offset;
}

VGAdev vgavmwaredev = {
	"vmware",

	0,
	0,
	0,
	vmwarelinear,
	vmwaredrawinit,
	0,
	0,
	0,
	vmwareflush,
};

VGAcur vgavmwarecur = {
	"vmwarehwgc",

	vmwareenable,
	vmwaredisable,
	vmwareload,
	vmwaremove,
};
