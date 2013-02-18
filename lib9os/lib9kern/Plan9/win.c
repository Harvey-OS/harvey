#include	"dat.h"
#include	"fns.h"
#include	"kernel.h"
#include	"error.h"

#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"keyboard.h"

enum
{
	Margin	= 4,
	Lsize		= 100,
};

extern Memimage *screenimage;

extern int kproc1(char*, void (*)(void*), void*, int);

static	ulong*	attachwindow(Rectangle*, ulong*, int*, int*);

static void	plan9readmouse(void*);
static void	plan9readkeybd(void*);
static int	mapspecials(char *s1, char *s2, int *n);

int	usenewwin = 1;
int	kbdiscons;
static int	truedepth;

static int		datafd;
static int		ctlfd;
static int		mousefd = -1;
static int		keybdfd;
static int		mousepid = -1;
static int		keybdpid = -1;
static int		cursfd;
static char	winname[64];

/* Following updated by attachwindow() asynchronously */
static QLock		ql;
static Rectangle	tiler;
static ulong*		data;
static uchar*		loadbuf;
static int		cursfd;
static int		imageid;
static Rectangle	imager;
static uchar	*chunk;
static int	chunksize;
static	int	dispbufsize;

#define	NINFO	12*12
#define	HDR		21


void
killrefresh(void)
{
	if(mousepid < 0)
		return;
	close(mousefd);
	close(ctlfd);
	close(datafd);
	postnote(PNPROC, mousepid, Eintr);
	postnote(PNPROC, keybdpid, Eintr);
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int *d, int *width, int *softscreen)
{
	int fd;
	char *p, buf[128], info[NINFO+1];

	if(usenewwin){
		p = getenv("wsys");
		if(p == nil)
			return nil;

		fd = open(p, ORDWR);
		if(fd < 0) {
			fprint(2, "attachscreen: can't open window manager: %r\n");
			return nil;
		}
		sprint(buf, "new -dx %d -dy %d", Xsize+2*Margin, Ysize+2*Margin);
		if(mount(fd, -1, "/mnt/wsys", MREPL, buf) < 0) {
			fprint(2, "attachscreen: can't mount window manager: %r\n");
			return nil;
		}
		if(bind("/mnt/wsys", "/dev", MBEFORE) < 0){
			fprint(2, "attachscreen: can't bind /mnt/wsys before /dev: %r\n");
			return nil;
		}
	}

	cursfd = open("/dev/cursor", OWRITE);
	if(cursfd < 0) {
		fprint(2, "attachscreen: open cursor: %r\n");
		return nil;
	}
	
	/* Set up graphics window console (chars->gkbdq) */
	keybdfd = open("/dev/cons", OREAD);
	if(keybdfd < 0) {
		fprint(2, "attachscreen: open keyboard: %r\n");
		return nil;
	}
	mousefd = open("/dev/mouse", ORDWR);
	if(mousefd < 0){
		fprint(2, "attachscreen: can't open mouse: %r\n");
		return nil;
	}
	if(usenewwin || 1){
		fd = open("/dev/consctl", OWRITE);
		if(fd < 0)
			fprint(2, "attachscreen: open /dev/consctl: %r\n");
		if(write(fd, "rawon", 5) != 5)
			fprint(2, "attachscreen: write /dev/consctl: %r\n");
	}

	/* Set up graphics files */
	ctlfd = open("/dev/draw/new", ORDWR);
	if(ctlfd < 0){
		fprint(2, "attachscreen: can't open graphics control file: %r\n");
		return nil;
	}
	if(read(ctlfd, info, sizeof info) < NINFO){
		close(ctlfd);
		fprint(2, "attachscreen: can't read graphics control file: %r\n");
		return nil;
	}
	sprint(buf, "/dev/draw/%d/data", atoi(info+0*12));
	datafd = open(buf, ORDWR|OCEXEC);
	if(datafd < 0){
		close(ctlfd);
		fprint(2, "attachscreen: can't read graphics data file: %r\n");
		return nil;
	}
	dispbufsize = iounit(datafd);
	if(dispbufsize <= 0)
		dispbufsize = 8000;
	if(dispbufsize < 512){
		close(ctlfd);
		close(datafd);
		fprint(2, "attachscreen: iounit %d too small\n", dispbufsize);
		return nil;
	}
	chunksize = dispbufsize - 64;

	if(attachwindow(r, chan, d, width) == nil){
		close(ctlfd);
		close(datafd);
		return nil;
	}
	
	mousepid = kproc1("readmouse", plan9readmouse, nil, 0);
	keybdpid = kproc1("readkbd", plan9readkeybd, nil, 0);

	fd = open("/dev/label", OWRITE);
	if(fd >= 0){
		snprint(buf, sizeof(buf), "inferno %d", getpid());
		write(fd, buf, strlen(buf));
		close(fd);
	}

	*softscreen = 1;
	return (uchar*)data;
}

static ulong*
attachwindow(Rectangle *r, ulong *chan, int *d, int *width)
{
	int n, fd, nb;
	char buf[256];
	uchar ubuf[128];
	ulong imagechan;

	/*
	 * Discover name of window
	 */
	fd = open("/mnt/wsys/winname", OREAD);
	if(fd<0 || (n=read(fd, winname, sizeof winname))<=0){
		fprint(2, "attachwindow: can only run inferno under rio, not stand-alone\n");
		return nil;
	}
	close(fd);
	/*
	 * If had previous window, release it
	 */
	if(imageid > 0){
		ubuf[0] = 'f';
		BPLONG(ubuf+1, imageid);
		if(write(datafd, ubuf, 1+4) != 1+4)
			fprint(2, "attachwindow: cannot free old window: %r\n");
	}
	/*
	 * Allocate image pointing to window, and discover its ID
	 */
	ubuf[0] = 'n';
	++imageid;
	BPLONG(ubuf+1, imageid);
	ubuf[5] = n;
	memmove(ubuf+6, winname, n);
	if(write(datafd, ubuf, 6+n) != 6+n){
		fprint(2, "attachwindow: cannot bind %d to window id '%s': %r\n", imageid, winname);
		return nil;
	}
	if(read(ctlfd, buf, sizeof buf) < 12*12){
		fprint(2, "attachwindow: cannot read window id: %r\n");
		return nil;
	}
	imagechan = strtochan(buf+2*12);
	truedepth = chantodepth(imagechan);
	if(truedepth == 0){
		fprint(2, "attachwindow: cannot handle window depth specifier %.12s\n", buf+2*12);
		return nil;
	}

	/*
	 * Report back
	 */
	if(chan != nil)
		*chan = imagechan;
	if(d != nil)
		*d = chantodepth(imagechan);
	nb = 0;
	if(r != nil){
		Xsize = atoi(buf+6*12)-atoi(buf+4*12)-2*Margin;
		Ysize = atoi(buf+7*12)-atoi(buf+5*12)-2*Margin;
		r->min.x = 0;
		r->min.y = 0;
		r->max.x = Xsize;
		r->max.y = Ysize;
		nb = bytesperline(*r, truedepth);
		data = malloc(nb*Ysize);
		loadbuf = malloc(nb*Lsize+1);
		chunk = malloc(HDR+chunksize+5);	/* +5 for flush (1 old, 5 new) */
	}
	imager.min.x = atoi(buf+4*12);
	imager.min.y = atoi(buf+5*12);
	imager.max.x = atoi(buf+6*12);
	imager.max.y = atoi(buf+7*12);

	if(width != nil)
		*width = nb/4;

	tiler.min.x = atoi(buf+4*12)+Margin;
	tiler.min.y = atoi(buf+5*12)+Margin;
	tiler.max.x = atoi(buf+6*12)-Margin;
	tiler.max.y = atoi(buf+7*12)-Margin;

	return data;
}

static int
plan9loadimage(Rectangle r, uchar *data, int ndata)
{
	long dy;
	int n, bpl;

	if(!rectinrect(r, imager)){
		werrstr("loadimage: bad rectangle");
		return -1;
	}
	bpl = bytesperline(r, truedepth);
	n = bpl*Dy(r);
	if(n > ndata){
		werrstr("loadimage: insufficient data");
		return -1;
	}
	ndata = 0;
	while(r.max.y > r.min.y){
		dy = r.max.y - r.min.y;
		if(dy*bpl> chunksize)
			dy = chunksize/bpl;
		n = dy*bpl;
		chunk[0] = 'y';
		BPLONG(chunk+1, imageid);
		BPLONG(chunk+5, r.min.x);
		BPLONG(chunk+9, r.min.y);
		BPLONG(chunk+13, r.max.x);
		BPLONG(chunk+17, r.min.y+dy);
		memmove(chunk+21, data, n);
		ndata += n;
		data += n;
		r.min.y += dy;
		n += 21;
		if(r.min.y >= r.max.y)	/* flush to screen */
			chunk[n++] = 'v';
		if(write(datafd, chunk, n) != n)
			return -1;
	}
	return ndata;
}

static void
_flushmemscreen(Rectangle r)
{
	int n, dy, l;
	Rectangle rr;

	if(data == nil || loadbuf == nil || chunk==nil)
		return;
	if(!rectclip(&r, Rect(0, 0, Xsize, Ysize)))
		return;
	if(!rectclip(&r, Rect(0, 0, Dx(tiler), Dy(tiler))))
		return;
	if(Dx(r)<=0 || Dy(r)<=0)
		return;
	l = bytesperline(r, truedepth);
	while(r.min.y < r.max.y){
		dy = Dy(r);
		if(dy > Lsize)
			dy = Lsize;
		rr = r;
		rr.max.y = rr.min.y+dy;
		n = unloadmemimage(screenimage, rr, loadbuf, l*dy);
		/* offset from (0,0) to window */
		rr.min.x += tiler.min.x;
		rr.min.y += tiler.min.y;
		rr.max.x += tiler.min.x;
		rr.max.y += tiler.min.y;
		if(plan9loadimage(rr, loadbuf, n) != n)
			fprint(2, "flushmemscreen: %d bytes: %r\n", n);
		r.min.y += dy;
	}
}

void
flushmemscreen(Rectangle r)
{
	qlock(&ql);
	_flushmemscreen(r);
	qunlock(&ql);
}

void
drawcursor(Drawcursor *c)
{
	int j, i, h, w, bpl;
	uchar *bc, *bs, *cclr, *cset, curs[2*4+2*2*16];

	/* Set the default system cursor */
	if(c->data == nil) {
		write(cursfd, curs, 0);
		return;
	}

	BPLONG(curs+0*4, c->hotx);
	BPLONG(curs+1*4, c->hoty);

	w = (c->maxx-c->minx);
	h = (c->maxy-c->miny)/2;

	cclr = curs+2*4;
	cset = curs+2*4+2*16;
	bpl = bytesperline(Rect(c->minx, c->miny, c->maxx, c->maxy), 1);
	bc = c->data;
	bs = c->data + h*bpl;

	if(h > 16)
		h = 16;
	if(w > 16)
		w = 16;
	w /= 8;
	for(i = 0; i < h; i++) {
		for(j = 0; j < w; j++) {
			cclr[j] = bc[j];
			cset[j] = bs[j];
		}
		bc += bpl;
		bs += bpl;
		cclr += 2;
		cset += 2;
	}
	write(cursfd, curs, sizeof curs);
}

static int
checkmouse(char *buf, int n)
{
	int x, y, tick, b;
	static int lastb, lastt, lastx, lasty, lastclick;

	switch(n){
	default:
		kwerrstr("atomouse: bad count");
		return -1;

	case 1+4*12:
		if(buf[0] == 'r'){
			qlock(&ql);
			if(attachwindow(nil, nil, nil, nil) == nil) {
				qunlock(&ql);
				return -1;
			}
			_flushmemscreen(Rect(0, 0, Xsize, Ysize));
			qunlock(&ql);
		}
		x = atoi(buf+1+0*12) - tiler.min.x;
		y = atoi(buf+1+1*12) - tiler.min.y;
		b = atoi(buf+1+2*12);
		tick = atoi(buf+1+3*12);
		if(b && lastb == 0){	/* button newly pressed */
			if(b==lastclick && tick-lastt<400
			   && abs(x-lastx)<10 && abs(y-lasty)<10)
				b |= (1<<8);
			lastt = tick;
			lastclick = b&0xff;
			lastx = x;
			lasty = y;
		}
		lastb = b&0xff;
		//mouse.msec = tick;
		mousetrack(b, x, y, 0);
		return n;
	}
}

static void
plan9readmouse(void *v)
{
	int n;
	char buf[128];

	USED(v);
	for(;;){
		n = read(mousefd, buf, sizeof(buf));
		if(n < 0)	/* probably interrupted */
			_exits(0);
		checkmouse(buf, n);
	}
}

static void
plan9readkeybd(void*)
{
	int n, partial;
	char buf[32];
	char dbuf[32 * 3];		/* overestimate but safe */

	partial = 0;
	for(;;){
		n = read(keybdfd, buf + partial, sizeof(buf) - partial);
		if(n < 0)	/* probably interrupted */
			_exits(0);
		partial += n;
		n = mapspecials(dbuf, buf, &partial);
		qproduce(gkbdq, dbuf, n);
	}
}

void
setpointer(int x, int y)
{
	char buf[50];
	int n;

	if(mousefd < 0)
		return;
	x += tiler.min.x;
	y += tiler.min.y;
	n = snprint(buf, sizeof buf, "m%11d %11d ", x, y);
	write(mousefd, buf, n);
}

/*
 * plan9 keyboard codes; from /sys/include/keyboard.h; can't include directly
 * because constant names clash.
 */
enum {
	P9KF=	0xF000,	/* Rune: beginning of private Unicode space */
	P9Spec=	0xF800,
	/* KF|1, KF|2, ..., KF|0xC is F1, F2, ..., F12 */
	Khome=	P9KF|0x0D,
	Kup=	P9KF|0x0E,
	Kpgup=	P9KF|0x0F,
	Kprint=	P9KF|0x10,
	Kleft=	P9KF|0x11,
	Kright=	P9KF|0x12,
	Kdown=	P9Spec|0x00,
	Kview=	P9Spec|0x00,
	Kpgdown=	P9KF|0x13,
	Kins=	P9KF|0x14,
	Kend=	KF|0x18,

	Kalt=		P9KF|0x15,
	Kshift=	P9KF|0x16,
	Kctl=		P9KF|0x17,
};

/*
 * translate plan 9 special characters from s2 (of length *n) into s1;
 * return number of chars placed into s1.
 * any trailing incomplete chars are moved to the beginning of s2,
 * and *n set to the number moved there.
 */
static int
mapspecials(char *s1, char *s2, int *n)
{
	char *s, *d, *es2;
	Rune r;
	d = s1;
	s = s2;
	es2 = s2 + *n;
	while (fullrune(s, es2 - s)) {
		s += chartorune(&r, s);
		switch (r) {
		case Kshift:
			r = LShift;
			break;
		case Kctl:
			r = LCtrl;
			break;
		case Kalt:
			r = LAlt;
			break;
		case Khome:
			r = Home;
			break;
		case Kend:
			r = End;
			break;
		case Kup:
			r = Up;
			break;
		case Kdown:
			r = Down;
			break;
		case Kleft:
			r = Left;
			break;
		case Kright:
			r = Right;
			break;
		case Kpgup:
			r = Pgup;
			break;
		case Kpgdown:
			r = Pgdown;
			break;
		case Kins:
			r = Ins;
			break;
		/*
		 * function keys
		 */
		case P9KF|1:
		case P9KF|2:
		case P9KF|3:
		case P9KF|4:
		case P9KF|5:
		case P9KF|6:
		case P9KF|7:
		case P9KF|8:
		case P9KF|9:
		case P9KF|10:
		case P9KF|11:
		case P9KF|12:
			r = (r - P9KF) + KF;
		}
		d += runetochar(d, &r);
	}
	*n = es2 - s;
	memmove(s2, s, *n);
	return d - s1;
}
