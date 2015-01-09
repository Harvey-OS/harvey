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
	addvgaseg("igfxmmio", p->mem[0].bar&~0x0F, p->mem[1].size);
	if(scr->paddr == 0)
		vgalinearpci(scr);
	if(scr->apsize)
		addvgaseg("igfxscreen", scr->paddr, scr->apsize);
}

VGAdev vgaigfxdev = {
	"igfx",
	igfxenable,
};
