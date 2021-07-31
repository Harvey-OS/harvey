#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"screen.h"

typedef struct Mouseinfo Mouseinfo;

struct Mouseinfo
{
	Point	xy;
	int	redraw;		/* update cursor on screen */
};

Mouseinfo	mouse;
Cursorinfo	cursor;
int		mouseshifted;
Cursor		curs;

void	Cursortocursor(Cursor*);
int	mousechanged(void*);
static void mouseclock(void);

enum{
	Qdir,
	Qcursor,
	Qmousepoint,
};

static Dirtab mousedir[]={
	".",	{Qdir, 0, QTDIR},	0,			DMDIR|0555,
	"cursor",	{Qcursor},	0,			0660,
	"mousepoint",	{Qmousepoint},	0,			0220,
};

extern	Memimage*	gscreen;

static void
mousereset(void)
{
	if(!conf.monitor)
		return;

	curs = arrow;
	Cursortocursor(&arrow);
	/* redraw cursor about 30 times per second */
	addclock0link(mouseclock, 33);
}

static void
mouseinit(void)
{
	if(!conf.monitor)
		return;

	cursoron(1);
}

static Chan*
mouseattach(char *spec)
{
	if(!conf.monitor)
		error(Egreg);
	return devattach('m', spec);
}

static Walkqid*
mousewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, mousedir, nelem(mousedir), devgen);
}

static int
mousestat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, mousedir, nelem(mousedir), devgen);
}

static Chan*
mouseopen(Chan *c, int omode)
{
	switch((ulong)c->qid.path){
	case Qdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qmousepoint:
		if(omode != OWRITE)
			error(Eperm);
		break;
	default:
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
mouseclose(Chan*)
{
}

static long
mouseread(Chan *c, void *va, long n, vlong off)
{
	uchar *p;

	p = va;
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, mousedir, nelem(mousedir), devgen);

	case Qcursor:
		if(off != 0)
			return 0;
		if(n < 2*4+2*2*16)
			error(Eshort);
		n = 2*4+2*2*16;
		lock(&cursor);
		BPLONG(p+0, curs.offset.x);
		BPLONG(p+4, curs.offset.y);
		memmove(p+8, curs.clr, 2*16);
		memmove(p+40, curs.set, 2*16);
		unlock(&cursor);
		return n;

	default:
		error(Egreg);
	}
	return 0;
}

static long
mousewrite(Chan *c, void *va, long n, vlong)
{
	char *p;
	Point pt;
	char buf[64];

	p = va;
	switch((ulong)c->qid.path){
	case Qdir:
		error(Eisdir);

	case Qcursor:
		cursoroff(1);
		if(n < 2*4+2*2*16){
			curs = arrow;
			Cursortocursor(&arrow);
		}else{
			n = 2*4+2*2*16;
			curs.offset.x = BGLONG(p+0);
			curs.offset.y = BGLONG(p+4);
			memmove(curs.clr, p+8, 2*16);
			memmove(curs.set, p+40, 2*16);
			Cursortocursor(&curs);
		}
		mouse.redraw = 1;
		mouseclock();
		cursoron(1);
		return n;

	case Qmousepoint:
		if(n > sizeof buf-1)
			n = sizeof buf -1;
		memmove(buf, va, n);
		buf[n] = 0;
		p = 0;
		pt.x = strtoul(buf, &p, 0);
		if(p == 0)
			error(Eshort);
		pt.y = strtoul(p, 0, 0);
		/*
		 * this used to be qlocked here but not locked below.
		 */
		if(gscreen && ptinrect(pt, gscreen->r)){
			mouse.xy = pt;
			mouse.redraw = 1;
			mouseclock();
		}
		return n;
	}

	error(Egreg);
	return -1;
}

Dev nmousedevtab = {
	'm',
	"mouse",

	mousereset,
	mouseinit,
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

void
Cursortocursor(Cursor *c)
{
	lock(&cursor);
	memmove(&cursor.Cursor, c, sizeof(Cursor));
	setcursor(c);
	unlock(&cursor);
}

/*
 *  called by the clock routine to redraw the cursor
 */
static void
mouseclock(void)
{
	if(mouse.redraw && canlock(&cursor)){
		mouse.redraw = 0;
		cursoroff(0);
		mouse.redraw = cursoron(0);
		unlock(&cursor);
	}
	drawactive(0);
}

Point
mousexy(void)
{
	return mouse.xy;
}
