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
	Qvgabios,
	Qvgactl,
	Qvgaovl,
	Qvgaovlctl,
};

static Dirtab vgadir[] = {
	".",	{ Qdir, 0, QTDIR },		0,	0550,
	"vgabios",	{ Qvgabios, 0 },	0x100000, 0440,
	"vgactl",		{ Qvgactl, 0 },		0,	0660,
	"vgaovl",		{ Qvgaovl, 0 },		0,	0660,
	"vgaovlctl",	{ Qvgaovlctl, 0 },	0, 	0660,
};

enum {
	CMactualsize,
	CMblank,
	CMblanktime,
	CMdrawinit,
	CMhwaccel,
	CMhwblank,
	CMhwgc,
	CMlinear,
	CMpalettedepth,
	CMpanning,
	CMsize,
	CMtextmode,
	CMtype,
	CMunblank,
};

static Cmdtab vgactlmsg[] = {
	CMactualsize,	"actualsize",	2,
	CMblank,	"blank",	1,
	CMblanktime,	"blanktime",	2,
	CMdrawinit,	"drawinit",	1,
	CMhwaccel,	"hwaccel",	2,
	CMhwblank,	"hwblank",	2,
	CMhwgc,		"hwgc",		2,
	CMlinear,	"linear",	0,
	CMpalettedepth,	"palettedepth",	2,
	CMpanning,	"panning",	2,
	CMsize,		"size",		3,
	CMtextmode,	"textmode",	1,
	CMtype,		"type",		2,
	CMunblank,	"unblank",	1,
};

static void
vgareset(void)
{
	/* reserve the 'standard' vga registers */
	if(ioalloc(0x2b0, 0x2df-0x2b0+1, 0, "vga") < 0)
		panic("vga ports already allocated"); 
	if(ioalloc(0x3c0, 0x3da-0x3c0+1, 0, "vga") < 0)
		panic("vga ports already allocated"); 
	conf.monitor = 1;
}

static Chan*
vgaattach(char* spec)
{
	if(*spec && strcmp(spec, "0"))
		error(Eio);
	return devattach('v', spec);
}

Walkqid*
vgawalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, vgadir, nelem(vgadir), devgen);
}

static int
vgastat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, vgadir, nelem(vgadir), devgen);
}

static Chan*
vgaopen(Chan* c, int omode)
{
	VGAscr *scr;
	static char *openctl = "openctl\n";

	scr = &vgascreen[0];
	if ((ulong)c->qid.path == Qvgaovlctl) {
		if (scr->dev && scr->dev->ovlctl)
			scr->dev->ovlctl(scr, c, openctl, strlen(openctl));
		else 
			error(Enonexist);
	}
	return devopen(c, omode, vgadir, nelem(vgadir), devgen);
}

static void
vgaclose(Chan* c)
{
	VGAscr *scr;
	static char *closectl = "closectl\n";

	scr = &vgascreen[0];
	if((ulong)c->qid.path == Qvgaovlctl)
		if(scr->dev && scr->dev->ovlctl){
			if(waserror()){
				print("ovlctl error: %s\n", up->errstr);
				return;
			}
			scr->dev->ovlctl(scr, c, closectl, strlen(closectl));
			poperror();
		}
}

static void
checkport(int start, int end)
{
	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df+1)
		return;
	if(start >= 0x3c0 && end <= 0x3da+1)
		return;

	if(iounused(start, end))
		return;
	error(Eperm);
}

static long
vgaread(Chan* c, void* a, long n, vlong off)
{
	int len;
	char *p, *s;
	VGAscr *scr;
	ulong offset = off;
	char chbuf[30];

	switch((ulong)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, vgadir, nelem(vgadir), devgen);

	case Qvgabios:
		if(offset >= 0x100000)
			return 0;
		if(offset+n >= 0x100000)
			n = 0x100000 - offset;
		memmove(a, (uchar*)kaddr(0)+offset, n);
		return n;

	case Qvgactl:
		scr = &vgascreen[0];

		p = malloc(READSTR);
		if(waserror()){
			free(p);
			nexterror();
		}

		len = 0;

		if(scr->dev)
			s = scr->dev->name;
		else
			s = "cga";
		len += snprint(p+len, READSTR-len, "type %s\n", s);

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
		USED(len);

		n = readstr(offset, a, n, p);
		poperror();
		free(p);

		return n;

	case Qvgaovl:
	case Qvgaovlctl:
		error(Ebadusefd);
		break;

	default:
		error(Egreg);
		break;
	}

	return 0;
}

static char Ebusy[] = "vga already configured";

static void
vgactl(Cmdbuf *cb)
{
	int align, i, size, x, y, z;
	char *chanstr, *p;
	ulong chan;
	Cmdtab *ct;
	VGAscr *scr;
	extern VGAdev *vgadev[];
	extern VGAcur *vgacur[];

	scr = &vgascreen[0];
	ct = lookupcmd(cb, vgactlmsg, nelem(vgactlmsg));
	switch(ct->index){
	case CMhwgc:
		if(strcmp(cb->f[1], "off") == 0){
			lock(&cursor);
			if(scr->cur){
				if(scr->cur->disable)
					scr->cur->disable(scr);
				scr->cur = nil;
			}
			unlock(&cursor);
			return;
		}
		if(strcmp(cb->f[1], "soft") == 0){
			lock(&cursor);
			swcursorinit();
			if(scr->cur && scr->cur->disable)
				scr->cur->disable(scr);
			scr->cur = &swcursor;
			if(scr->cur->enable)
				scr->cur->enable(scr);
			unlock(&cursor);
			return;
		}
		for(i = 0; vgacur[i]; i++){
			if(strcmp(cb->f[1], vgacur[i]->name))
				continue;
			lock(&cursor);
			if(scr->cur && scr->cur->disable)
				scr->cur->disable(scr);
			scr->cur = vgacur[i];
			if(scr->cur->enable)
				scr->cur->enable(scr);
			unlock(&cursor);
			return;
		}
		break;

	case CMtype:
		for(i = 0; vgadev[i]; i++){
			if(strcmp(cb->f[1], vgadev[i]->name))
				continue;
			if(scr->dev && scr->dev->disable)
				scr->dev->disable(scr);
			scr->dev = vgadev[i];
			if(scr->dev->enable)
				scr->dev->enable(scr);
			return;
		}
		break;

	case CMtextmode:
		screeninit();
		return;

	case CMsize:
		x = strtoul(cb->f[1], &p, 0);
		if(x == 0 || x > 10240)
			error(Ebadarg);
		if(*p)
			p++;

		y = strtoul(p, &p, 0);
		if(y == 0 || y > 10240)
			error(Ebadarg);
		if(*p)
			p++;

		z = strtoul(p, &p, 0);

		chanstr = cb->f[2];
		if((chan = strtochan(chanstr)) == 0)
			error("bad channel");

		if(chantodepth(chan) != z)
			error("depth, channel do not match");

		cursoroff(1);
		deletescreenimage();
		if(screensize(x, y, z, chan))
			error(Egreg);
		vgascreenwin(scr);
		resetscreenimage();
		cursoron(1);
		return;

	case CMactualsize:
		if(scr->gscreen == nil)
			error("set the screen size first");

		x = strtoul(cb->f[1], &p, 0);
		if(x == 0 || x > 2048)
			error(Ebadarg);
		if(*p)
			p++;

		y = strtoul(p, nil, 0);
		if(y == 0 || y > 2048)
			error(Ebadarg);

		if(x > scr->gscreen->r.max.x || y > scr->gscreen->r.max.y)
			error("physical screen bigger than virtual");

		physgscreenr = Rect(0,0,x,y);
		scr->gscreen->clipr = physgscreenr;
		return;
	
	case CMpalettedepth:
		x = strtoul(cb->f[1], &p, 0);
		if(x != 8 && x != 6)
			error(Ebadarg);

		scr->palettedepth = x;
		return;

	case CMdrawinit:
		if(scr->gscreen == nil)
			error("drawinit: no gscreen");
		if(scr->dev && scr->dev->drawinit)
			scr->dev->drawinit(scr);
		return;
	
	case CMlinear:
		if(cb->nf!=2 && cb->nf!=3)
			error(Ebadarg);
		size = strtoul(cb->f[1], 0, 0);
		if(cb->nf == 2)
			align = 0;
		else
			align = strtoul(cb->f[2], 0, 0);
		if(screenaperture(size, align) < 0)
			error("not enough free address space");
		return;
/*	
	case CMmemset:
		memset((void*)strtoul(cb->f[1], 0, 0), atoi(cb->f[2]), atoi(cb->f[3]));
		return;
*/

	case CMblank:
		drawblankscreen(1);
		return;
	
	case CMunblank:
		drawblankscreen(0);
		return;
	
	case CMblanktime:
		blanktime = strtoul(cb->f[1], 0, 0);
		return;

	case CMpanning:
		if(strcmp(cb->f[1], "on") == 0){
			if(scr == nil || scr->cur == nil)
				error("set screen first");
			if(!scr->cur->doespanning)
				error("panning not supported");
			scr->gscreen->clipr = scr->gscreen->r;
			panning = 1;
		}
		else if(strcmp(cb->f[1], "off") == 0){
			scr->gscreen->clipr = physgscreenr;
			panning = 0;
		}else
			break;
		return;

	case CMhwaccel:
		if(strcmp(cb->f[1], "on") == 0)
			hwaccel = 1;
		else if(strcmp(cb->f[1], "off") == 0)
			hwaccel = 0;
		else
			break;
		return;
	
	case CMhwblank:
		if(strcmp(cb->f[1], "on") == 0)
			hwblank = 1;
		else if(strcmp(cb->f[1], "off") == 0)
			hwblank = 0;
		else
			break;
		return;
	}

	cmderror(cb, "bad VGA control message");
}

char Enooverlay[] = "No overlay support";

static long
vgawrite(Chan* c, void* a, long n, vlong off)
{
	ulong offset = off;
	Cmdbuf *cb;
	VGAscr *scr;

	switch((ulong)c->qid.path){

	case Qdir:
		error(Eperm);

	case Qvgactl:
		if(offset || n >= READSTR)
			error(Ebadarg);
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		vgactl(cb);
		poperror();
		free(cb);
		return n;

	case Qvgaovl:
		scr = &vgascreen[0];
		if (scr->dev == nil || scr->dev->ovlwrite == nil) {
			error(Enooverlay);
			break;
		}
		return scr->dev->ovlwrite(scr, a, n, off);

	case Qvgaovlctl:
		scr = &vgascreen[0];
		if (scr->dev == nil || scr->dev->ovlctl == nil) {
			error(Enooverlay);
			break;
		}
		scr->dev->ovlctl(scr, c, a, n);
		return n;

	default:
		error(Egreg);
		break;
	}

	return 0;
}

Dev vgadevtab = {
	'v',
	"vga",

	vgareset,
	devinit,
	devshutdown,
	vgaattach,
	vgawalk,
	vgastat,
	vgaopen,
	devcreate,
	vgaclose,
	vgaread,
	devbread,
	vgawrite,
	devbwrite,
	devremove,
	devwstat,
};
