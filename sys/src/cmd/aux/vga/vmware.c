#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

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
	Rhostbpp,
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

	Rpalette = 1024,
};	

typedef struct Vmware	Vmware;
struct Vmware {
	ulong	mmio;
	ulong	fb;

	ulong	ra;
	ulong	rd;

	ulong	r[Nreg];

	char	chan[32];
	int	depth;
};

static char*
rname[Nreg] = {
	"ID",
	"Enable",
	"Width",
	"Height",
	"MaxWidth",
	"MaxHeight",
	"Depth",
	"Bpp",
	"PseudoColor",
	"RedMask",
	"GreenMask",
	"BlueMask",
	"Bpl",
	"FbStart",
	"FbOffset",
	"FbMaxSize",
	"FbSize",
	"Cap",
	"MemStart",
	"MemSize",
	"ConfigDone",
	"Sync",
	"Busy",
	"GuestID",
	"CursorID",
	"CursorX",
	"CursorY",
	"CursorOn",
	"HostBpp",
};

static ulong
vmrd(Vmware *vm, int i)
{
	outportl(vm->ra, i);
	return inportl(vm->rd);
}

static void
vmwr(Vmware *vm, int i, ulong v)
{
	outportl(vm->ra, i);
	outportl(vm->rd, v);
}

static uint
bits(ulong a)
{
	int b;

	for(b=0; a; a>>=1)
		if(a&1)
			b++;
	return b;
}

static void
snarf(Vga* vga, Ctlr* ctlr)
{
	int extra, i;
	Pcidev *p;
	Vmware *vm;

	p = vga->pci;
	if(p == nil)
		error("%s: vga->pci not set\n", ctlr->name);

	vm = alloc(sizeof(Vmware));

	switch(p->did){
	case 0x710:	/* VMware video chipset #1 */
		vm->ra = 0x4560;
		vm->rd = 0x4560+4;
		break;

	case 0x405:	/* VMware video chipset #2, untested */
		vm->ra = p->mem[0].bar&~3;
		vm->rd = vm->ra+1;
		break;

	default:
		error("%s: unrecognized chipset %.4ux\n", ctlr->name, p->did);
	}

	for(i=0; i<Nreg; i++)
		vm->r[i] = vmrd(vm, i);

//vmwr(vm, Renable, 0);
	/*
	 * Figure out color channel.  Delay errors until init,
	 * which is after the register dump.
	 */
	vm->depth = vm->r[Rbpp];
	extra = vm->r[Rbpp] - vm->r[Rdepth];
	if(vm->r[Rrmask] > vm->r[Rgmask] && vm->r[Rgmask] > vm->r[Rbmask]){
		if(extra)
			sprint(vm->chan, "x%d", extra);
		else
			vm->chan[0] = '\0';
		sprint(vm->chan+strlen(vm->chan), "r%dg%db%d", bits(vm->r[Rrmask]),
			bits(vm->r[Rgmask]), bits(vm->r[Rbmask]));
	}else if(vm->r[Rbmask] > vm->r[Rgmask] && vm->r[Rgmask] > vm->r[Rrmask]){
		sprint(vm->chan, "b%dg%dr%d", bits(vm->r[Rbmask]),
			bits(vm->r[Rgmask]), bits(vm->r[Rrmask]));
		if(extra)
			sprint(vm->chan+strlen(vm->chan), "x%d", extra);
	}else
		sprint(vm->chan, "unknown");

	/* Record the frame buffer start, size */
	vga->vmb = vm->r[Rfbstart];
	vga->apz = vm->r[Rfbmaxsize];

	vga->private = vm;
	ctlr->flag |= Fsnarf;
}


static void
options(Vga*, Ctlr* ctlr)
{
	ctlr->flag |= Hlinear|Henhanced|Foptions;
}


static void
clock(Vga*, Ctlr*)
{
	/* BEST CLOCK ROUTINE EVER! */
}

static void
init(Vga* vga, Ctlr* ctlr)
{
	Vmware *vm;

	vm = vga->private;

	vmwr(vm, Rid, (0x900000<<8)|2);
	if(vmrd(vm, Rid)&0xFF != 2)
		error("old vmware svga version %lud; need version 2\n",
			vmrd(vm,Rid)&0xFF);

	ctlr->flag |= Ulinear;
	if(strcmp(vm->chan, "unknown") == 0)
		error("couldn't translate color masks into channel\n");

	/* Always use the screen depth, and clip the screen size */
	vga->mode->z = vm->r[Rbpp];
	if(vga->mode->x > vm->r[Rmaxwidth])
		vga->mode->x = vm->r[Rmaxwidth];
	if(vga->mode->y > vm->r[Rmaxheight])
		vga->mode->y = vm->r[Rmaxheight];

	vm->r[Rwidth] = vga->mode->x;
	vm->r[Rheight] = vga->mode->y;

	/* Figure out the channel string */
	strcpy(vga->mode->chan, vm->chan);

	/* Record the bytes per line */
	ctlr->flag |= Finit;
}

static void
load(Vga* vga, Ctlr *ctlr)
{
	char buf[64];
	int x;
	Vmware *vm;

	vm = vga->private;
	vmwr(vm, Rwidth, vm->r[Rwidth]);
	vmwr(vm, Rheight, vm->r[Rheight]);
	vmwr(vm, Renable, 1);
	vmwr(vm, Rguestid, 0x5010);	/* OS type is "Other" */

	x = vmrd(vm, Rbpl)/(vm->depth/8);
	if(x != vga->mode->x){
		vga->virtx = x;
		sprint(buf, "%ludx%ludx%d %s", vga->virtx, vga->virty,
			vga->mode->z, vga->mode->chan);
		vgactlw("size", buf);
	}
	ctlr->flag |= Fload;
}

static void
dump(Vga* vga, Ctlr* ctlr)
{
	int i;
	Vmware *vm;

	vm = vga->private;

	for(i=0; i<Nreg; i++){
		printitem(ctlr->name, rname[i]);
		Bprint(&stdout, " %.8lux\n", vm->r[i]);
	}

	printitem(ctlr->name, "chan");
	Bprint(&stdout, " %s\n", vm->chan);
	printitem(ctlr->name, "depth");
	Bprint(&stdout, " %d\n", vm->depth);
	printitem(ctlr->name, "linear");
	
}

Ctlr vmware = {
	"vmware",			/* name */
	snarf,				/* snarf */
	options,			/* options */
	init,				/* init */
	load,				/* load */
	dump,				/* dump */
};

Ctlr vmwarehwgc = {
	"vmwarehwgc",			/* name */
	0,				/* snarf */
	0,				/* options */
	0,				/* init */
	0,				/* load */
	0,				/* dump */
};

