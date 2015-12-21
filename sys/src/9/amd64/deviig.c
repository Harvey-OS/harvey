/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define I_AM_HERE print("Core 0 is in %s() at %s:%d\n", \
                         __FUNCTION__, __FILE__, __LINE__);
/*
 * VGA controller
 */
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
	Qdir,
	Qiigctl,
};

static Dirtab iigdir[] = {
	".",	{ Qdir, 0, QTDIR },		0,	0550,
	"iigctl",		{ Qiigctl, 0 },		0,	0660,
};

enum {
	CMsize,
	CMblank,
	CMunblank,
};

static Cmdtab iigctlmsg[] = {
	CMsize,		"size",		1,
	CMblank,  "blank", 1,
	CMunblank,  "unblank", 1,
};
typedef struct Iig Iig;

extern VGAscr vgascreen[];

static void
iigreset(void)
{
	uintptr_t p, s;
	int ix = 2;
	void *v;
	vgascreen[0].pci = pcimatch(nil, 0x8086, 0x0116);
	if (vgascreen[0].pci)
		print("Found sandybridge at 0x%p\n", vgascreen[0].pci->tbdf);
	else {
		print("NO sandybridge found\n");
		return;
	}
	/* on coreboot systems, which is what this is mainly for, the graphics memory is allocated and
	 * reserved. This little hack won't be permanent but we might as well see if we can get to that
	 * memory.
	 */
	p = vgascreen[0].pci->mem[ix].bar & ~0xfULL;
	if (! p) {
		print("NO PHYSADDR at BAR 0\n");
		return;
	}
	s = 4 * 1048576; //vgascreen[0].pci->mem[ix].size;
	print("paddr is %p size %x\n", p, s); 
	v = vmap(p, s);
	print("p is 0x%p, s is 0x%p, v is THIS! %p\n", p, s, v);
	if (v) {
		vgascreen[0].vaddr = v;
		vgascreen[0].paddr = p;
		vgascreen[0].apsize = s;
	}
}

static Chan*
iigattach(char* spec)
{
	if(*spec && strcmp(spec, "0"))
		error(Eio);
	return devattach('G', spec);
}

Walkqid*
iigwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, iigdir, nelem(iigdir), devgen);
}

static int
iigstat(Chan* c, unsigned char* dp, int n)
{
	return devstat(c, dp, n, iigdir, nelem(iigdir), devgen);
}

static Chan*
iigopen(Chan* c, int omode)
{
	return devopen(c, omode, iigdir, nelem(iigdir), devgen);
}

static void
iigclose(Chan* c)
{
}

static int32_t
iigread(Chan* c, void* a, int32_t n, int64_t off)
{
	VGAscr *scr;
	Proc *up = externup();
	int len;
	char *p, *s;
	uint32_t offset = off;
	char chbuf[30];
	int i;

	switch((uint32_t)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, iigdir, nelem(iigdir), devgen);

	case Qiigctl:
		scr = &vgascreen[0];

		p = malloc(READSTR);
		if(p == nil)
			error(Enomem);
		if(waserror()){
			free(p);
			nexterror();
		}

		len = 0;

		if(scr->dev)
			s = scr->dev->name;
		else
			s = "cga";
		len += snprint(p+len, READSTR-len, "type %s, paddr %p\n", s, scr->paddr);

		if(scr->gscreen) {
			len += snprint(p+len, READSTR-len, "size %dx%dx%d %s\n",
				scr->gscreen->r.max.x, scr->gscreen->r.max.y,
				scr->gscreen->depth, chantostr(chbuf, scr->gscreen->chan));

			if(Dx(scr->gscreen->r) != Dx(physgscreenr) 
			|| Dy(scr->gscreen->r) != Dy(physgscreenr))
				len += snprint(p+len, READSTR-len, "actualsize %dx%d\n",
					physgscreenr.max.x, physgscreenr.max.y);
		}

		len += snprint(p+len, READSTR-len, "blank time %lud idle %d state %s\n",
			blanktime, drawidletime(), scr->isblank ? "off" : "on");
		len += snprint(p+len, READSTR-len, "hwaccel %s\n", hwaccel ? "on" : "off");
		len += snprint(p+len, READSTR-len, "hwblank %s\n", hwblank ? "on" : "off");
		len += snprint(p+len, READSTR-len, "panning %s\n", panning ? "on" : "off");
		len += snprint(p+len, READSTR-len, "addr p 0x%lux v 0x%p size 0x%ux\n", scr->paddr, scr->vaddr, scr->apsize);
		for(i = 0; i < 6; i++)
			len += snprint(p+len, READSTR-len, "bar 0x%p 0x%lx\n", 
				vgascreen[0].pci->mem[i].bar, vgascreen[0].pci->mem[i].size);
		USED(len);

		n = readstr(offset, a, n, p);
		poperror();
		free(p);

		return n;

		break;

	default:
		error(Egreg);
		break;
	}

	return 0;
}

static void
iigctl(Cmdbuf *cb)
{
	Cmdtab *ct;
	uint32_t chan;
	char *chanstr = "r8g8b8";

	ct = lookupcmd(cb, iigctlmsg, nelem(iigctlmsg));
	switch(ct->index){
	case CMsize:
		if((chan = strtochan(chanstr)) == 0)
			error("bad channel");

		if(chantodepth(chan) != 24) {
			print("depth, channel do not match: chan %d chanstr %d chantodepth() %d\n", 
			      chan, chanstr, chantodepth(chan));
			error("depth, channel do not match");
		}
		
		
		{ uint32_t *v = vgascreen[0].vaddr; int i; for(i = 0; i < 512*1024; i++) v[i] = 0xff00;}
		return;
I_AM_HERE;
		if(screensize(1280, 850, 24, chan)) {
I_AM_HERE;
	  		print("SHIT: screen setup failed\n");
				error(Egreg);
		}
		print("base %x\n", vgascreen[0].paddr);
		{ uint32_t *v = vgascreen[0].vaddr; int i; for(i = 0; i < 512*1024; i++) v[i] = 0xff00;}
		return;
I_AM_HERE;
		vgascreenwin(&vgascreen[0]);
I_AM_HERE;
		resetscreenimage();
I_AM_HERE;
		cursoron(1);
I_AM_HERE;
		return;

	case CMblank:
		drawblankscreen(1);
		return;
	
	case CMunblank:
		drawblankscreen(0);
		return;
	}

	cmderror(cb, "bad IIG control message");
}

static int32_t
iigwrite(Chan* c, void* a, int32_t n, int64_t off)
{
	Proc *up = externup();
	Cmdbuf *cb;
	switch((uint32_t)c->qid.path){
	case Qdir:
		error(Eperm);

	case Qiigctl:
		if(off || n >= READSTR)
			error(Ebadarg);
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		iigctl(cb);
		poperror();
		free(cb);
		return n;
	default:
		error(Egreg);
		break;
	}

	return 0;
}

Dev iigdevtab = {
	'G',
	"iig",

	iigreset,
	devinit,
	devshutdown,
	iigattach,
	iigwalk,
	iigstat,
	iigopen,
	devcreate,
	iigclose,
	iigread,
	devbread,
	iigwrite,
	devbwrite,
	devremove,
	devwstat,
};
