#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "ureg.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"


static void *hardscreen;

#define WORD(p) ((p)[0] | ((p)[1]<<8))
#define LONG(p) ((p)[0] | ((p)[1]<<8) | ((p)[2]<<16) | ((p)[3]<<24))
#define PWORD(p, v) (p)[0] = (v); (p)[1] = (v)>>8
#define PLONG(p, v) (p)[0] = (v); (p)[1] = (v)>>8; (p)[2] = (v)>>16; (p)[3] = (v)>>24

extern void realmode(Ureg*);

static uchar*
vbesetup(Ureg *u, int ax)
{
	ulong pa;

	pa = PADDR(RMBUF);
	memset(u, 0, sizeof *u);
	u->ax = ax;
	u->es = (pa>>4)&0xF000;
	u->di = pa&0xFFFF;
	return (void*)RMBUF;
}

static void
vbecall(Ureg *u)
{
	u->trap = 0x10;
	realmode(u);
	if((u->ax&0xFFFF) != 0x004F)
		error("vesa bios error");
}

static void
vbecheck(void)
{
	Ureg u;
	uchar *p;

	p = vbesetup(&u, 0x4F00);
	strcpy((char*)p, "VBE2");
	vbecall(&u);
	if(memcmp((char*)p, "VESA", 4) != 0)
		error("bad vesa signature");
	if(p[5] < 2)
		error("bad vesa version");
}

static int
vbegetmode(void)
{
	Ureg u;

	vbesetup(&u, 0x4F03);
	vbecall(&u);
	return u.bx;
}

static uchar*
vbemodeinfo(int mode)
{
	uchar *p;
	Ureg u;

	p = vbesetup(&u, 0x4F01);
	u.cx = mode;
	vbecall(&u);
	return p;
}

static void
vesalinear(VGAscr *scr, int, int)
{
	int i, mode, size;
	uchar *p;
	ulong paddr;
	Pcidev *pci;

	vbecheck();
	mode = vbegetmode();
	/* bochs loses the top bits - cannot use this
	if((mode&(1<<14)) == 0)
		error("not in linear graphics mode");
	*/
	mode &= 0x3FFF;
	p = vbemodeinfo(mode);
	if(!(WORD(p+0) & (1<<4)))
		error("not in VESA graphics mode");
	if(!(WORD(p+0) & (1<<7)))
		error("not in linear graphics mode");

	paddr = LONG(p+40);
	size = WORD(p+20)*WORD(p+16);
	size = PGROUND(size);

	/*
	 * figure out max size of memory so that we have
	 * enough if the screen is resized.
	 */
	pci = nil;
	while((pci = pcimatch(pci, 0, 0)) != nil){
		if(pci->ccrb != Pcibcdisp)
			continue;
		for(i=0; i<nelem(pci->mem); i++)
			if(paddr == (pci->mem[i].bar&~0x0F)){
				if(pci->mem[i].size > size)
					size = pci->mem[i].size;
				goto havesize;
			}
	}

	/* no pci - heuristic guess */
	if(size < 4*1024*1024)
		size = 4*1024*1024;
	else
		size = ROUND(size, 1024*1024);

havesize:
	if(size > 16*1024*1024)		/* probably arbitrary; could increase */
		size = 16*1024*1024;
	vgalinearaddr(scr, paddr, size);
	hardscreen = scr->vaddr;
	mtrr(paddr, size, "wc");
}

static void
vesaflush(VGAscr *scr, Rectangle r)
{
	int t, w, wid, off;
	ulong *hp, *sp, *esp;

	if(rectclip(&r, scr->gscreen->r) == 0)
		return;

	hp = hardscreen;
	assert(hp != nil);
	sp = (ulong*)(scr->gscreendata->bdata + scr->gscreen->zero);
	t = (r.max.x * scr->gscreen->depth + 2*BI2WD-1) / BI2WD;
	w = (r.min.x * scr->gscreen->depth) / BI2WD;
	w = (t - w) * BY2WD;
	wid = scr->gscreen->width;
	off = r.min.y * wid + (r.min.x * scr->gscreen->depth) / BI2WD;

	hp += off;
	sp += off;
	esp = sp + Dy(r) * wid;
	while(sp < esp){
		memmove(hp, sp, w);
		hp += wid;
		sp += wid;
	}
}

VGAdev vgavesadev = {
	"vesa",
	0,
	0,
	0,
	vesalinear,
	0,
	0,
	0,
	0,
	vesaflush,
};
