#include <u.h>
#include <libc.h>
#include <draw.h>

Point	ZP;
Rectangle ZR;
Display	*display;
Font	*font;
Image	*screen;
int	_drawdebug;

static char deffontname[] = "*default*";
Screen	*_screen;

int		debuglockdisplay = 0;

int
Rconv(va_list *arg, Fconv *f)
{
	Rectangle r;
	char buf[128];

	r = va_arg(*arg, Rectangle);
	sprint(buf, "%P %P", r.min, r.max);
	strconv(buf, f);
	return 0;
}

int
Pconv(va_list *arg, Fconv *f)
{
	Point p;
	char buf[64];

	p = va_arg(*arg, Point);
	sprint(buf, "[%d %d]", p.x, p.y);
	strconv(buf, f);
	return 0;
}

static void
drawshutdown(void)
{
	Display *d;

	d = display;
	if(d){
		display = nil;
		closedisplay(d);
	}
}

int
geninitdraw(char *devdir, void(*error)(Display*, char*), char *fontname, char *label, char *windir, int ref)
{
	int fd, n;
	Subfont *df;
	char buf[128];

	display = initdisplay(devdir, windir, error);
	if(display == nil)
		return -1;

	/*
	 * Set up default font
	 */
	df = getdefont(display);
	display->defaultsubfont = df;
	if(df == nil){
		_drawprint(2, "imageinit: can't open default subfont: %r\n");
    Error:
		closedisplay(display);
		display = nil;
		return -1;
	}
	if(fontname == nil){
		fd = open("/env/font", OREAD);
		if(fd >= 0){
			n = read(fd, buf, sizeof(buf));
			if(n>0 && n<sizeof buf-1){
				buf[n] = 0;
				fontname = buf;
			}
			close(fd);
		}
	}
	/*
	 * Build fonts with caches==depth of screen, for speed.
	 * If conversion were faster, we'd use 0 and save memory.
	 */
	if(fontname == nil){
		snprint(buf, sizeof buf, "%d %d\n0 %d\t%s\n", df->height, df->ascent,
			df->n-1, deffontname);
//BUG: Need something better for this	installsubfont("*default*", df);
		font = buildfont(display, buf, deffontname);
		if(font == nil){
			_drawprint(2, "imageinit: can't open default font: %r\n");
			goto Error;
		}
	}else{
		font = openfont(display, fontname);	/* BUG: grey fonts */
		if(font == nil){
			_drawprint(2, "imageinit: can't open font %s: %r\n", fontname);
			goto Error;
		}
	}
	display->defaultfont = font;

	/*
	 * Write label; ignore errors (we might not be running under rio)
	 */
	if(label){
		snprint(buf, sizeof buf, "%s/label", display->windir);
		fd = open(buf, OREAD);
		if(fd >= 0){
			read(fd, display->oldlabel, (sizeof display->oldlabel)-1);
			close(fd);
			fd = create(buf, OWRITE, 0666);
			if(fd >= 0){
				write(fd, label, strlen(label));
				close(fd);
			}
		}
	}

	if(getwindow(display, ref) < 0)
		goto Error;

	atexit(drawshutdown);

	return 1;
}

int
initdraw(void(*error)(Display*, char*), char *fontname , char *label)
{
	char *dev = "/dev";
	char dir[DIRLEN];

	if(stat("/dev/draw/new", dir)<0 && bind("#i", "/dev", MAFTER)<0){
		_drawprint(2, "imageinit: can't bind /dev/draw: %r");
		return -1;
	}
	return geninitdraw(dev, error, fontname, label, dev, Refnone);
}

/*
 * Attach, or possibly reattach, to window.
 * If reattaching, maintain value of screen pointer.
 */
int
gengetwindow(Display *d, char *winname, Image **winp, Screen **scrp, int ref)
{
	int n, fd;
	char buf[64+1];
	Image *image;

	fd = open(winname, OREAD);
	if(fd<0 || (n=read(fd, buf, sizeof buf-1))<=0){
		*winp = d->image;
		assert(*winp && (*winp)->chan != 0);
		return 1;
	}
	close(fd);
	buf[n] = '\0';
	if(*winp != nil){
		_freeimage1(*winp);
		freeimage((*scrp)->image);
		freescreen(*scrp);
		*scrp = nil;
	}
	image = namedimage(d, buf);
	if(image == 0){
		*winp = nil;
		return -1;
	}
	assert(image->chan != 0);

	*scrp = allocscreen(image, d->white, 0);
	if(*scrp == nil){
		*winp = nil;
		return -1;
	}

	*winp = _allocwindow(*winp, *scrp, insetrect(image->r, Borderwidth), ref, DWhite);
	if(*winp == nil)
		return -1;
	assert((*winp)->chan != 0);
	return 1;
}

int
getwindow(Display *d, int ref)
{
	char winname[128];

	snprint(winname, sizeof winname, "%s/winname", d->windir);
	return gengetwindow(d, winname, &screen, &_screen, ref);
}

#define	NINFO	12*12

Display*
initdisplay(char *dev, char *win, void(*error)(Display*, char*))
{
	char buf[128], info[NINFO+1], *t;
	int datafd, ctlfd, reffd;
	Display *disp;
	Image *image;
	Dir dir;
	ulong chan;

	fmtinstall('P', Pconv);
	fmtinstall('R', Rconv);
	if(dev == 0)
		dev = "/dev";
	if(win == 0)
		win = "/dev";
	if(strlen(dev)>sizeof buf-25 || strlen(win)>sizeof buf-25){
		werrstr("initdisplay: directory name too long");
		return nil;
	}
	t = strdup(win);
	if(t == nil)
		return nil;

	sprint(buf, "%s/draw/new", dev);
	ctlfd = open(buf, ORDWR|OCEXEC);
	if(ctlfd < 0){
		if(bind("#i", dev, MAFTER) < 0){
    Error1:
			free(t);
			werrstr("initdisplay: %s: %r", buf);
			return 0;
		}
		ctlfd = open(buf, ORDWR|OCEXEC);
	}
	if(ctlfd < 0)
		goto Error1;
	if(read(ctlfd, info, sizeof info) < NINFO){
    Error2:
		close(ctlfd);
		goto Error1;
	}

	if((chan=strtochan(info+2*12)) == 0){
		werrstr("bad channel in %s", buf);
		goto Error2;
	}

	sprint(buf, "%s/draw/%d/data", dev, atoi(info+0*12));
	datafd = open(buf, ORDWR|OCEXEC);
	if(datafd < 0)
		goto Error2;
	sprint(buf, "%s/draw/%d/refresh", dev, atoi(info+0*12));
	reffd = open(buf, OREAD|OCEXEC);
	if(reffd < 0){
    Error3:
		close(datafd);
		goto Error2;
	}
	disp = malloc(sizeof(Display));
	if(disp == 0){
    Error4:
		close(reffd);
		goto Error3;
	}
	image = malloc(sizeof(Image));
	if(image == 0){
    Error5:
		free(disp);
		goto Error4;
	}
	memset(image, 0, sizeof(Image));
	memset(disp, 0, sizeof(Display));
	image->display = disp;
	image->id = 0;
	image->chan = chan;
	image->depth = chantodepth(chan);
	image->repl = atoi(info+3*12);
	image->r.min.x = atoi(info+4*12);
	image->r.min.y = atoi(info+5*12);
	image->r.max.x = atoi(info+6*12);
	image->r.max.y = atoi(info+7*12);
	image->clipr.min.x = atoi(info+8*12);
	image->clipr.min.y = atoi(info+9*12);
	image->clipr.max.x = atoi(info+10*12);
	image->clipr.max.y = atoi(info+11*12);
	disp->dirno = atoi(info+0*12);
	disp->fd = datafd;
	disp->ctlfd = ctlfd;
	disp->reffd = reffd;
	disp->image = image;
	disp->bufp = disp->buf;
	disp->error = error;
	disp->chan = image->chan;
	disp->depth = image->depth;
	disp->windir = t;
	disp->devdir = strdup(dev);
	qlock(&disp->qlock);
	disp->white = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DWhite);
	disp->black = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DBlack);
	if(disp->white == nil || disp->black == nil){
		free(image);
		free(disp->devdir);
		free(disp->white);
		free(disp->black);
		goto Error5;
	}
	disp->opaque = disp->white;
	disp->transparent = disp->black;
	if(dirfstat(ctlfd, &dir)>=0 && dir.type=='i'){
		disp->local = 1;
		disp->dataqid = dir.qid.path;
	}

	assert(disp->chan != 0 && image->chan != 0);
	return disp;
}

/*
 * Call with d unlocked.
 * Note that disp->defaultfont and defaultsubfont are not freed here.
 */
void
closedisplay(Display *disp)
{
	int fd;
	char buf[128];

	if(disp == nil)
		return;
	if(disp == display)
		display = nil;
	if(disp->oldlabel[0]){
		snprint(buf, sizeof buf, "%s/label", disp->windir);
		fd = open(buf, OWRITE);
		if(fd >= 0){
			write(fd, disp->oldlabel, strlen(disp->oldlabel));
			close(fd);
		}
	}

	free(disp->devdir);
	free(disp->windir);
	freeimage(disp->white);
	freeimage(disp->black);
	free(disp->image);
	close(disp->fd);
	close(disp->ctlfd);
	/* should cause refresh slave to shut down */
	close(disp->reffd);
	qunlock(&disp->qlock);
	free(disp);
}

void
lockdisplay(Display *disp)
{
	if(debuglockdisplay){
		/* avoid busy looping; it's rare we collide anyway */
		while(!canqlock(&disp->qlock)){
			_drawprint(1, "proc %d waiting for display lock...\n", getpid());
			sleep(1000);
		}
	}else
		qlock(&disp->qlock);
}

void
unlockdisplay(Display *disp)
{
	qunlock(&disp->qlock);
}

/* use static buffer to avoid stack bloat */
int
_drawprint(int fd, char *fmt, ...)
{
	int n;
	va_list arg;
	static char buf[1024];
	static QLock l;

	qlock(&l);
	va_start(arg, fmt);
	doprint(buf, buf+sizeof buf, fmt, arg);
	va_end(arg);
	n = write(fd, buf, strlen(buf));
	qunlock(&l);
	return n;
}

void
drawerror(Display *d, char *s)
{
	char err[ERRLEN];

	if(d->error)
		d->error(d, s);
	else{
		errstr(err);
		_drawprint(2, "draw: %s: %s\n", s, err);
		exits(s);
	}
}

static
int
doflush(Display *d)
{
	int n;

	n = d->bufp-d->buf;
	if(n <= 0)
		return 1;

	if(write(d->fd, d->buf, n) != n){
		if(_drawdebug)
			_drawprint(2, "flushimage fail: d=%p: %r\n", d); /**/
		d->bufp = d->buf;	/* might as well; chance of continuing */
		return -1;
	}
	d->bufp = d->buf;
	return 1;
}

int
flushimage(Display *d, int visible)
{
	if(visible)
		*d->bufp++ = 'v';	/* one byte always reserved for this */
	return doflush(d);
}

uchar*
bufimage(Display *d, int n)
{
	uchar *p;

	if(n<0 || n>Displaybufsize){
		werrstr("bad count in bufimage");
		return 0;
	}
	if(d->bufp+n > d->buf+Displaybufsize)
		if(doflush(d) < 0)
			return 0;
	p = d->bufp;
	d->bufp += n;
	return p;
}

