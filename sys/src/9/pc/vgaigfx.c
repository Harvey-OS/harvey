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

static uintptr
igfxcuralloc(Pcidev *pci, void *mmio, int apsize)
{
	int n;
	u32int pa, *buf, *p, *e;

	buf = mallocalign(64*64*4, BY2PG, 0, 0);
	if(buf == nil){
		print("igfx: no memory for cursor image\n");
		return 0;
	}
	n = (apsize > 128*MB ? 128*1024 : apsize/1024) / 4 - 4;
	p = (u32int*)((uchar*)mmio + pci->mem[0].size/2) + n;
	*(u32int*)((uchar*)mmio + 0x2170) = 0;	/* flush write buffers */
	for(e=p+4, pa=PADDR(buf); p<e; p++, pa+=1<<12)
		*p = pa | 1;
	*(u32int*)((uchar*)mmio + 0x2170) = 0;	/* flush write buffers */
	return (uintptr)n << 12;
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
	if(p->mem[0].bar & 1)
		return;
	scr->mmio = vmap(p->mem[0].bar&~0x0F, p->mem[0].size);
	if(scr->mmio == nil)
		return;
	addvgaseg("igfxmmio", p->mem[0].bar&~0x0F, p->mem[0].size);
	if(scr->paddr == 0)
		vgalinearpci(scr);
	if(scr->apsize){
		addvgaseg("igfxscreen", scr->paddr, scr->apsize);
		scr->storage = igfxcuralloc(p, scr->mmio, scr->apsize);
	}
	scr->softscreen = 1;
}

static void
igfxblank(VGAscr *scr, int blank)
{
	u32int off;

	switch(scr->pci->did){
	default:
		return;

	case 0x2a02:	/* GM965 */
	case 0x2a42:	/* GM45 */
	case 0x2592:	/* GM915 */
		off = 0x61204;
		break;

	case 0x0126:	/* SNB */
	case 0x0166:	/* IVB */
		off = 0xC7204;
		break;
	}

	/* toggle PP_CONTROL backlight & power state */
	if(blank)
		scr->mmio[off/4] &= ~0x5;
	else
		scr->mmio[off/4] |= 0x5;
}

static void
igfxdrawinit(VGAscr *scr)
{
	scr->blank = igfxblank;
}

VGAdev vgaigfxdev = {
	"igfx",
	igfxenable,
	nil,
	nil,
	nil,
	igfxdrawinit,
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
	case 0x0126:	/* Sandy Bridge HD Graphics 3000 */
		if(pipe > 2)
			return nil;
		break;
	case 0x2592:	/* GM915 */
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
	igfxcurload(scr, &cursor);
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
