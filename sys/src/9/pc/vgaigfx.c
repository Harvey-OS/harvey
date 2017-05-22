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

static ulong
stolenmb(Pcidev *p)
{
	switch(p->did){
	case 0x0412:	/* Haswell HD Graphics 4600 */
	case 0x0a16:	/* Haswell HD Graphics 4400 */
	case 0x0166:	/* Ivy Bridge */
	case 0x0102:	/* Core-5 Sandy Bridge */
	case 0x0152:	/* Core-i3 */
		switch((pcicfgr16(p, 0x50) >> 3) & 0x1f){
		case 0x01:	return 32  - 2;
		case 0x02:	return 64  - 2;		/* 0102 Dell machine here */
		case 0x03:	return 96  - 2;
		case 0x04:	return 128 - 2;
		case 0x05:	return 32  - 2;
		case 0x06:	return 48  - 2;
		case 0x07:	return 64  - 2;
		case 0x08:	return 128 - 2;
		case 0x09:	return 256 - 2;
		case 0x0A:	return 96  - 2;
		case 0x0B:	return 160 - 2;
		case 0x0C:	return 224 - 2;
		case 0x0D:	return 352 - 2;
		case 0x0E:	return 448 - 2;
		case 0x0F:	return 480 - 2;
		case 0x10:	return 512 - 2;
		}
		break;
	case 0x2a42:	/* X200 */
	case 0x29a2:	/* 82P965/G965 HECI desktop */
	case 0x2a02:	/* CF-R7 */
		switch((pcicfgr16(p, 0x52) >> 4) & 7){
		case 0x01:	return 1;
		case 0x02:	return 4;
		case 0x03:	return 8;
		case 0x04:	return 16;
		case 0x05:	return 32;
		case 0x06:	return 48;
		case 0x07:	return 64;
		}
		break;
	}
	return 0;
}

static uintptr
gmsize(Pcidev *pci, void *mmio)
{
	u32int x, i, npg, *gtt;

	npg = stolenmb(pci)<<(20-12);
	if(npg == 0)
		return 0;
	gtt = (u32int*)((uchar*)mmio + pci->mem[0].size/2);
	if((gtt[0]&1) == 0)
		return 0;
	x = (gtt[0]>>12)+1;
	for(i=1; i<npg; i++){
		if((gtt[i]&1) == 0 || (gtt[i]>>12) != x)
			break;
		x++;
	}
	if(0) print("igfx: graphics memory at %p-%p (%ud MB)\n", 
		(uintptr)(x-i)<<12, (uintptr)x<<12, (i>>(20-12)));
	return (uintptr)i<<12;
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
		scr->storage = gmsize(p, scr->mmio);
		if(scr->storage < MB)
			scr->storage = 0;
		else if(scr->storage > scr->apsize)
			scr->storage = scr->apsize;
		if(scr->storage != 0)
			scr->storage -= PGROUND(64*64*4);
	}
	scr->softscreen = 1;
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

	NPIPE = 4,
};

static u32int*
igfxcurregs(VGAscr* scr, int pipe)
{
	u32int o;

	if(scr->mmio == nil || scr->storage == 0)
		return nil;
	o = pipe == 3 ? 0xf000 : pipe*0x1000;
	/* check PIPExCONF if enabled */
	if((scr->mmio[(0x70008 | o)/4] & (1<<31)) == 0)
		return nil;
	switch(scr->pci->did){
	case 0x0412:	/* Haswell HD Graphics 4600 */
	case 0x0a16:	/* Haswell HD Graphics 4400 */
		if(pipe > 3)
			return nil;
		if(pipe == 3)
			o = 0;
		break;
	case 0x0166:	/* Ivy Bridge */
	case 0x0152:	/* Core-i3 */
		if(pipe > 2)
			return nil;
		break;
	case 0x2a42:	/* X200 */
	case 0x29a2:	/* 82P965/G965 HECI desktop */
	case 0x2a02:	/* CF-R7 */
	case 0x0102:	/* Sndy Bridge */
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
