/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"screen.h"

typedef struct Mouseinfo	Mouseinfo;
typedef struct Mousestate	Mousestate;

struct Mousestate
{
	Point	xy;			/* mouse.xy */
	int	buttons;		/* mouse.buttons */
	uint32_t	counter;	/* increments every update */
	uint32_t	msec;	/* time of last event */
};

struct Mouseinfo
{
	Mousestate	mst;
	int	dx;
	int	dy;
	int	track;		/* dx & dy updated */
	int	redraw;		/* update cursor on screen */
	uint32_t	lastcounter;	/* value when /dev/mouse read */
	Rendez	r;
	Ref	ref;
	QLock	qlock;
	int	open;
	int	acceleration;
	int	maxacc;
	Mousestate 	queue[16];	/* circular buffer of click events */
	int	ri;	/* read index into queue */
	int	wi;	/* write index into queue */
	uint8_t	qfull;	/* queue is full */
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
	Qmouse,
};

static Dirtab mousedir[]={
	{".",	{Qdir, 0, QTDIR},	0,			DMDIR|0555},
	{"cursor",	{Qcursor},	0,			0666},
	{"mouse",	{Qmouse},	0,			0666},
};

static uint8_t buttonmap[8] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};

extern	Memimage*	gscreen;
extern	void mousewarpnote(Point);

static void
mousereset(void)
{
	curs = arrow;
	Cursortocursor(&arrow);
}

static void
mouseinit(void)
{
	cursoron(1);
}

static Chan*
mouseattach(char *spec)
{
	return devattach('m', spec);
}

static Walkqid*
mousewalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;

	wq = devwalk(c, nc, name, nname, mousedir, nelem(mousedir), devgen);
	if(wq != nil && wq->clone != c && (wq->clone->qid.type&QTDIR)==0)
		incref(&mouse.ref);
	return wq;
}

static int
mousestat(Chan *c, uint8_t *db, int n)
{
	return devstat(c, db, n, mousedir, nelem(mousedir), devgen);
}

static Chan*
mouseopen(Chan *c, int omode)
{
	switch((uint32_t)c->qid.path){
	case Qdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qmouse:
		lock(&mouse.ref.lock);
		if(mouse.open){
			unlock(&mouse.ref.lock);
			error(Einuse);
		}
		mouse.open = 1;
		mouse.ref.ref++;
		unlock(&mouse.ref.lock);
		break;
	default:
		incref(&mouse.ref);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
mousecreate(Chan *c, char *a, int i, uint32_t u)
{
	error(Eperm);
}

static void
mouseclose(Chan *c)
{
	if((c->qid.type&QTDIR)==0 && (c->flag&COPEN)){
		lock(&mouse.ref.lock);
		if(c->qid.path == Qmouse)
			mouse.open = 0;
		if(--mouse.ref.ref == 0){
			cursoroff(1);
			curs = arrow;
			Cursortocursor(&arrow);
			cursoron(1);
		}
		unlock(&mouse.ref.lock);
	}
}


static int32_t
mouseread(Chan *c, void *va, int32_t n, int64_t off)
{
	char buf[4*12+1];
	uint8_t *p;
	uint32_t offset = off;
	Mousestate m;
	int b;

	p = va;
	switch((uint32_t)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, mousedir, nelem(mousedir), devgen);

	case Qcursor:
		if(offset != 0)
			return 0;
		if(n < 2*4+2*2*16)
			error(Eshort);
		n = 2*4+2*2*16;
		lock(&cursor.lock);
		BPLONG(p+0, curs.offset.x);
		BPLONG(p+4, curs.offset.y);
		memmove(p+8, curs.clr, 2*16);
		memmove(p+40, curs.set, 2*16);
		unlock(&cursor.lock);
		return n;

	case Qmouse:
		while(mousechanged(0) == 0)
			rendsleep(&mouse.r, mousechanged, 0);

		mouse.qfull = 0;

		/*
		 * No lock of the indicies is necessary here, because ri is only
		 * updated by us, and there is only one mouse reader
		 * at a time.  I suppose that more than one process
		 * could try to read the fd at one time, but such behavior
		 * is degenerate and already violates the calling
		 * conventions for sleep above.
		 */
		if(mouse.ri != mouse.wi){
			m = mouse.queue[mouse.ri];
			if(++mouse.ri == nelem(mouse.queue))
				mouse.ri = 0;
		} else {
			lock(&cursor.lock);

			m = mouse.mst;
			unlock(&cursor.lock);
		}

		b = buttonmap[m.buttons&7];
		/* put buttons 4 and 5 back in */
		b |= m.buttons & (3<<3);
		sprint(buf, "m%11d %11d %11d %11lu",
			m.xy.x, m.xy.y,
			b,
			m.msec);
		mouse.lastcounter = m.counter;
		if(n > 1+4*12)
			n = 1+4*12;
		memmove(va, buf, n);
		return n;
	}
	return 0;
}

static int32_t
mousewrite(Chan *c, void *va, int32_t n, int64_t m)
{
	char *p;
	Point pt;
	char buf[64];

	p = va;
	switch((uint32_t)c->qid.path){
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
		qlock(&mouse.qlock);
		mouse.redraw = 1;
		mouseclock();
		qunlock(&mouse.qlock);
		cursoron(1);
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
		qlock(&mouse.qlock);
		if(ptinrect(pt, gscreen->r)){
			mousetrack(pt.x, pt.y, mouse.mst.buttons, nsec()/(1000*1000LL));
			mousewarpnote(pt);
		}
		qunlock(&mouse.qlock);
		return n;
	}

	error(Egreg);
	return -1;
}

Dev mousedevtab = {
	'm',
	"mouse",

	mousereset,
	mouseinit,
	mouseattach,
	mousewalk,
	mousestat,
	mouseopen,
	mousecreate,
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
	lock(&cursor.lock);
	memmove(&cursor.cursor, c, sizeof(Cursor));
	setcursor(c);
	unlock(&cursor.lock);
}

static void
mouseclock(void)
{
	lock(&cursor.lock);
	if(mouse.redraw){
		mouse.redraw = 0;
		cursoroff(0);
		mouse.redraw = cursoron(0);
	}
	unlock(&cursor.lock);
}

/*
 *  called at interrupt level to update the structure and
 *  awaken any waiting procs.
 */
void
mousetrack(int x, int y, int b, int msec)
{
	int lastb;

	lastb = mouse.mst.buttons;
	mouse.mst.xy = Pt(x, y);
	mouse.mst.buttons = b;
	mouse.redraw = 1;
	mouse.mst.counter++;
	mouse.mst.msec = msec;

	/*
	 * if the queue fills, we discard the entire queue and don't
	 * queue any more events until a reader polls the mouse.
	 */
	if(!mouse.qfull && lastb != b){	/* add to ring */
		mouse.queue[mouse.wi] = mouse.mst;
		if(++mouse.wi == nelem(mouse.queue))
			mouse.wi = 0;
		if(mouse.wi == mouse.ri)
			mouse.qfull = 1;
	}
	rendwakeup(&mouse.r);
	mouseclock();
}

int
mousechanged(void *v)
{
	return mouse.lastcounter != mouse.mst.counter;
}

Point
mousexy(void)
{
	return mouse.mst.xy;
}

void
mouseaccelerate(int x)
{
	mouse.acceleration = x;
	if(mouse.acceleration < 3)
		mouse.maxacc = 2;
	else
		mouse.maxacc = mouse.acceleration;
}
