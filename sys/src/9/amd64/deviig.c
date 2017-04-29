/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	{".",		{ Qdir, 0, QTDIR },	0,	0550},
	{"iigctl",	{ Qiigctl, 0 },		0,	0660},
};

enum {
	CMsize,
	CMblank,
	CMunblank,
};

static Cmdtab iigctlmsg[] = {
	{CMsize,	"size",		1},
	{CMblank,	"blank",	1},
	{CMunblank,	"unblank",	1},
};
typedef struct Iig Iig;

struct Iig {
	struct VGAscr scr;
};
static Iig iig;

static void
iigreset(void)
{
	iig.scr.pci = pcimatch(nil, 0x8086, 0x0116);
	if (iig.scr.pci)
		print("Found sandybridge at 0x%x\n", iig.scr.pci->tbdf);
	else {
		print("NO sandybridge found\n");
		return;
	}
	/* on coreboot systems, which is what this is mainly for, the graphics memory is allocated and
	 * reserved. This little hack won't be permanent but we might as well see if we can get to that
	 * memory.
	 */
	void *x = vmap(iig.scr.pci->mem[1].bar, iig.scr.pci->mem[1].size);
	print("x is THIS! %p\n", x);
	if (x) {
		iig.scr.vaddr = x;
		iig.scr.paddr = iig.scr.pci->mem[1].bar;
		iig.scr.apsize = iig.scr.pci->mem[1].size;
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

	switch((uint32_t)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, iigdir, nelem(iigdir), devgen);

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

	ct = lookupcmd(cb, iigctlmsg, nelem(iigctlmsg));
	switch(ct->index){
	case CMsize:
		error("nope");
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
	.dc = 'G',
	.name = "iig",

	.reset = iigreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = iigattach,
	.walk = iigwalk,
	.stat = iigstat,
	.open = iigopen,
	.create = devcreate,
	.close = iigclose,
	.read = iigread,
	.bread = devbread,
	.write = iigwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
