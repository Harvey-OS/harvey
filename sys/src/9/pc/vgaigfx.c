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

#define MB	0x100000

static ulong
preallocsize(Pcidev *p)
{
	switch(p->did){
	case 0x0166:	/* Ivy Bridge */
		switch((pcicfgr16(p, 0x50) >> 3) & 0x1f){
		case 0x01:	return 32*MB	- 2*MB;
		case 0x02:	return 64*MB	- 2*MB;
		case 0x03:	return 96*MB	- 2*MB;
		case 0x04:	return 128*MB	- 2*MB;
		case 0x05:	return 32*MB	- 2*MB;
		case 0x06:	return 48*MB	- 2*MB;
		case 0x07:	return 64*MB	- 2*MB;
		case 0x08:	return 128*MB	- 2*MB;
		case 0x09:	return 256*MB	- 2*MB;
		case 0x0A:	return 96*MB	- 2*MB;
		case 0x0B:	return 160*MB	- 2*MB;
		case 0x0C:	return 224*MB	- 2*MB;
		case 0x0D:	return 352*MB	- 2*MB;
		case 0x0E:	return 448*MB	- 2*MB;
		case 0x0F:	return 480*MB	- 2*MB;
		case 0x10:	return 512*MB	- 2*MB;
		}
		break;
	case 0x2a42:	/* X200 */
		switch((pcicfgr16(p, 0x52) >> 4) & 7){
		case 0x01:	return 1*MB;
		case 0x02:	return 4*MB;
		case 0x03:	return 8*MB;
		case 0x04:	return 16*MB;
		case 0x05:	return 32*MB;
		case 0x06:	return 48*MB;
		case 0x07:	return 64*MB;
		}
		break;
	}
	return 0;
}

static void
igfxenable(VGAscr* scr)
{
	Pcidev *p;
	
	if(scr->mmio != nil)
		return;
	p = scr->pci;
	if(p == nil)
		return;
	scr->mmio = vmap(p->mem[0].bar&~0x0F, p->mem[0].size);
	if(scr->mmio == nil)
		return;
	addvgaseg("igfxmmio", p->mem[0].bar&~0x0F, p->mem[0].size);
	if(scr->paddr == 0)
		vgalinearpci(scr);
	if(scr->apsize){
		addvgaseg("igfxscreen", scr->paddr, scr->apsize);
		scr->storage = preallocsize(p);
		if(scr->storage > scr->apsize)
			scr->storage = scr->apsize;
		if(scr->storage != 0)
			scr->storage -= PGROUND(64*64*4);
	}
}

VGAdev vgaigfxdev = {
	"igfx",
	igfxenable,
};

static void
igfxcurload(VGAscr* scr, Cursor* curs)
{
	uchar set, clr;
	u32int *p;
	int i, j;

	if(scr->storage == 0)
		return;
	p = (u32int*)((uchar*)scr->vaddr + scr->storage);
	memset(p, 0, 64*64*4);
	for(i=0;i<32;i++) {
		set = curs->set[i];
		clr = curs->clr[i];
		for(j=0x80; j; j>>=1){
			if((set|clr)&j)
				*p++ = (0xFF<<24) | (set&j ? 0x000000 : 0xFFFFFF);
			else
				*p++ = 0;
		}
		if(i & 1)
			p += 64-16;
	}
	scr->offset = curs->offset;
}

enum {
	CURCTL = 0,
	CURBASE,
	CURPOS,

	NPIPE = 3,
};

static u32int*
igfxcurregs(VGAscr* scr, int pipe)
{
	u32int o;

	if(scr->mmio == nil || scr->storage == 0)
		return nil;
	o = pipe*0x1000;
	/* check PIPExCONF if enabled */
	if((scr->mmio[(0x70008 | o)/4] & (1<<31)) == 0)
		return nil;
	switch(scr->pci->did){
	case 0x0116:	/* Ivy Bridge */
		if(pipe > 2)
			return nil;
		break;
	case 0x2a42:	/* X200 */
		if(pipe > 1)
			return nil;
		o = pipe*0x40;
		break;
	default:
		if(pipe > 0)
			return nil;
	}
	return (u32int*)((uchar*)scr->mmio + (0x70080 + o));
}

static int
igfxcurmove(VGAscr* scr, Point p)
{
	int i, x, y;
	u32int *r;

	for(i=0; i<NPIPE; i++){
		if((r = igfxcurregs(scr, i)) != nil){
			x = p.x + scr->offset.x;
			if(x < 0) x = -x | 0x8000;
			y = p.y + scr->offset.y;
			if(y < 0) y = -y | 0x8000;
			r[CURPOS] = (y << 16) | x;
		}
	}
	return 0;
}

static void
igfxcurenable(VGAscr* scr)
{
	u32int *r;
	int i;

	igfxenable(scr);
	igfxcurload(scr, &arrow);
	igfxcurmove(scr, ZP);

	for(i=0; i<NPIPE; i++){
		if((r = igfxcurregs(scr, i)) != nil){
			r[CURCTL] = (r[CURCTL] & ~(3<<28 | 1<<5)) | (i<<28) | 7;
			r[CURBASE] = scr->storage;
		}
	}
}

static void
igfxcurdisable(VGAscr* scr)
{
	u32int *r;
	int i;

	for(i=0; i<NPIPE; i++){
		if((r = igfxcurregs(scr, i)) != nil){
			r[CURCTL] &= ~(1<<5 | 7);
			r[CURBASE] = 0;
		}
	}
}

VGAcur vgaigfxcur = {
	"igfxhwgc",
	igfxcurenable,
	igfxcurdisable,
	igfxcurload,
	igfxcurmove,
};
