#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#include	"draw.h"
#include	"memdraw.h"
#include	"screen.h"

int	mousequeue = 1;

Mouseinfo	mouse;
Cursorinfo	cursor;

static int	mousechanged(void*);

enum{
	Qdir,
	Qcursor,
	Qmouse
};

Dirtab mousedir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,	
	"cursor",	{Qcursor},	0,			0666,
	"mouse",	{Qmouse},	0,			0666,
};

#define	NMOUSE	(sizeof(mousedir)/sizeof(Dirtab))

static Chan*
mouseattach(char *spec)
{
	return devattach('m', spec);
}

static Walkqid*
mousewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, mousedir, NMOUSE, devgen);
}

static int
mousestat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, mousedir, NMOUSE, devgen);
}

static Chan*
mouseopen(Chan *c, int omode)
{
	switch((long)c->qid.path){
	case Qdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qmouse:
		lock(&mouse.lk);
		if(mouse.open){
			unlock(&mouse.lk);
			error(Einuse);
		}
		mouse.open = 1;
		unlock(&mouse.lk);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
mouseclose(Chan *c)
{
	if(!(c->flag&COPEN))
		return;

	switch((long)c->qid.path) {
	case Qmouse:
		lock(&mouse.lk);
		mouse.open = 0;
		unlock(&mouse.lk);
		cursorarrow();
	}
}


long
mouseread(Chan *c, void *va, long n, vlong offset)
{
	char buf[4*12+1];
	uchar *p;
	int i, nn;
	ulong msec;
/*	static int map[8] = {0, 4, 2, 6, 1, 5, 3, 7 };	*/

	p = va;
	switch((long)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, mousedir, NMOUSE, devgen);

	case Qcursor:
		if(offset != 0)
			return 0;
		if(n < 2*4+2*2*16)
			error(Eshort);
		n = 2*4+2*2*16;
		lock(&cursor.lk);
		BPLONG(p+0, cursor.offset.x);
		BPLONG(p+4, cursor.offset.y);
		memmove(p+8, cursor.clr, 2*16);
		memmove(p+40, cursor.set, 2*16);
		unlock(&cursor.lk);
		return n;

	case Qmouse:
		while(mousechanged(0) == 0)
			sleep(&mouse.r, mousechanged, 0);

		lock(&screen.lk);
		if(screen.reshaped) {
			screen.reshaped = 0;
			sprint(buf, "t%11d %11d", 0, ticks());
			if(n > 1+2*12)
				n = 1+2*12;
			memmove(va, buf, n);
			unlock(&screen.lk);
			return n;
		}
		unlock(&screen.lk);

		lock(&mouse.lk);
		i = mouse.ri;
		nn = (mouse.wi + Mousequeue - i) % Mousequeue;
		if(nn < 1)
			panic("empty mouse queue");
		msec = ticks();
		while(nn > 1) {
			if(mouse.queue[i].msec + Mousewindow > msec)
				break;
			i = (i+1)%Mousequeue;
			nn--;
		}
		sprint(buf, "m%11d %11d %11d %11d",
			mouse.queue[i].xy.x,
			mouse.queue[i].xy.y,
			mouse.queue[i].buttons,
			mouse.queue[i].msec);
		mouse.ri = (i+1)%Mousequeue;
		unlock(&mouse.lk);
		if(n > 1+4*12)
			n = 1+4*12;
		memmove(va, buf, n);
		return n;
	}
	return 0;
}

long
mousewrite(Chan *c, void *va, long n, vlong offset)
{
	char *p;
	Point pt;
	char buf[64];

	USED(offset);

	p = va;
	switch((long)c->qid.path){
	case Qdir:
		error(Eisdir);

	case Qcursor:
		if(n < 2*4+2*2*16){
			cursorarrow();
		}else{
			n = 2*4+2*2*16;
			lock(&cursor.lk);
			cursor.offset.x = BGLONG(p+0);
			cursor.offset.y = BGLONG(p+4);
			memmove(cursor.clr, p+8, 2*16);
			memmove(cursor.set, p+40, 2*16);
			unlock(&cursor.lk);
			setcursor();
		}
		return n;

	case Qmouse:
		if(n > sizeof buf-1)
			n = sizeof buf -1;
		memmove(buf, va, n);
		buf[n] = 0;
		p = 0;
		pt.x = strtoul(buf+1, &p, 0);
		if(p == 0)
			error(Eshort);
		pt.y = strtoul(p, 0, 0);
		if(ptinrect(pt, gscreen->r))
			mouseset(pt);
		return n;
	}

	error(Egreg);
	return -1;
}

int
mousechanged(void *a)
{
	USED(a);

	return mouse.ri != mouse.wi || screen.reshaped;
}

Dev mousedevtab = {
	'm',
	"mouse",

	devreset,
	devinit,
	devshutdown,
	mouseattach,
	mousewalk,
	mousestat,
	mouseopen,
	devcreate,
	mouseclose,
	mouseread,
	devbread,
	mousewrite,
	devbwrite,
	devremove,
	devwstat,
};

