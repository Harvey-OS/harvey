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
	ulong	counter;	/* increments every update */
	ulong	msec;	/* time of last event */
};

struct Mouseinfo
{
	Mousestate;
	int	dx;
	int	dy;
	int	track;		/* dx & dy updated */
	int	redraw;		/* update cursor on screen */
	ulong	lastcounter;	/* value when /dev/mouse read */
	Rendez	r;
	Ref;
	QLock;
	int	open;
	int	acceleration;
	int	maxacc;
	Mousestate 	queue[16];	/* circular buffer of click events */
	int	ri;	/* read index into queue */
	int	wi;	/* write index into queue */
	uchar	qfull;	/* queue is full */
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
	".",	{Qdir, 0, QTDIR},	0,			DMDIR|0555,
	"cursor",	{Qcursor},	0,			0666,
	"mouse",	{Qmouse},	0,			0666,
};

static uchar buttonmap[8] = {
	0, 1, 2, 3, 4, 5, 6, 7,
};
static int mouseswap;

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
		incref(&mouse);
	return wq;
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
	case Qmouse:
		lock(&mouse);
		if(mouse.open){
			unlock(&mouse);
			error(Einuse);
		}
		mouse.open = 1;
		mouse.ref++;
		unlock(&mouse);
		break;
	default:
		incref(&mouse);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
mousecreate(Chan*, char*, int, ulong)
{
	error(Eperm);
}

static void
mouseclose(Chan *c)
{
	if((c->qid.type&QTDIR)==0 && (c->flag&COPEN)){
		lock(&mouse);
		if(c->qid.path == Qmouse)
			mouse.open = 0;
		if(--mouse.ref == 0){
			cursoroff(1);
			curs = arrow;
			Cursortocursor(&arrow);
			cursoron(1);
		}
		unlock(&mouse);
	}
}


static long
mouseread(Chan *c, void *va, long n, vlong off)
{
	char buf[4*12+1];
	uchar *p;
	static int map[8] = {0, 4, 2, 6, 1, 5, 3, 7 };
	ulong offset = off;
	Mousestate m;
	int b;

	p = va;
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, mousedir, nelem(mousedir), devgen);

	case Qcursor:
		if(offset != 0)
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
			lock(&cursor);
	
			m = mouse.Mousestate;
			unlock(&cursor);
		}

		b = buttonmap[m.buttons&7];
		/* put buttons 4 and 5 back in */
		b |= m.buttons & (3<<3);
		sprint(buf, "m%11d %11d %11d %11lud",
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

static void
setbuttonmap(char* map)
{
	int i, x, one, two, three;

	one = two = three = 0;
	for(i = 0; i < 3; i++){
		if(map[i] == 0)
			error(Ebadarg);
		if(map[i] == '1'){
			if(one)
				error(Ebadarg);
			one = 1<<i;
		}
		else if(map[i] == '2'){
			if(two)
				error(Ebadarg);
			two = 1<<i;
		}
		else if(map[i] == '3'){
			if(three)
				error(Ebadarg);
			three = 1<<i;
		}
		else
			error(Ebadarg);
	}
	if(map[i])
		error(Ebadarg);

	memset(buttonmap, 0, 8);
	for(i = 0; i < 8; i++){
		x = 0;
		if(i & 1)
			x |= one;
		if(i & 2)
			x |= two;
		if(i & 4)
			x |= three;
		buttonmap[x] = i;
	}
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
		qlock(&mouse);
		mouse.redraw = 1;
		mouseclock();
		qunlock(&mouse);
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
		qlock(&mouse);
		if(ptinrect(pt, gscreen->r)){
			mousetrack(pt.x, pt.y, mouse.buttons, nsec()/(1000*1000LL));
			mousewarpnote(pt);
		}
		qunlock(&mouse);
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
	lock(&cursor);
	memmove(&cursor.Cursor, c, sizeof(Cursor));
	setcursor(c);
	unlock(&cursor);
}

static int
scale(int x)
{
	int sign = 1;

	if(x < 0){
		sign = -1;
		x = -x;
	}
	switch(x){
	case 0:
	case 1:
	case 2:
	case 3:
		break;
	case 4:
		x = 6 + (mouse.acceleration>>2);
		break;
	case 5:
		x = 9 + (mouse.acceleration>>1);
		break;
	default:
		x *= mouse.maxacc;
		break;
	}
	return sign*x;
}

static void
mouseclock(void)
{
	lock(&cursor);
	if(mouse.redraw){
		mouse.redraw = 0;
		cursoroff(0);
		mouse.redraw = cursoron(0);
	}
	unlock(&cursor);
}

/*
 *  called at interrupt level to update the structure and
 *  awaken any waiting procs.
 */
void
mousetrack(int x, int y, int b, int msec)
{
	int lastb;

	lastb = mouse.buttons;
	mouse.xy = Pt(x, y);
	mouse.buttons = b;
	mouse.redraw = 1;
	mouse.counter++;
	mouse.msec = msec;

	/*
	 * if the queue fills, we discard the entire queue and don't
	 * queue any more events until a reader polls the mouse.
	 */
	if(!mouse.qfull && lastb != b){	/* add to ring */
		mouse.queue[mouse.wi] = mouse.Mousestate;
		if(++mouse.wi == nelem(mouse.queue))
			mouse.wi = 0;
		if(mouse.wi == mouse.ri)
			mouse.qfull = 1;
	}
	rendwakeup(&mouse.r);
	mouseclock();
}

int
mousechanged(void*)
{
	return mouse.lastcounter != mouse.counter;
}

Point
mousexy(void)
{
	return mouse.xy;
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
