#include <u.h>
#include <libc.h>
#include <draw.h>

Display	*display;
Font	*font;
Image	*screen;
int	_drawdebug = 0;

static char deffontname[] = "*default*";
Screen	*_screen;

int		debuglockdisplay = 0;

static void _closedisplay(Display*, int);

/* note handler */
static void
drawshutdown(void)
{
	Display *d;

	d = display;
	if(d){
		display = nil;
		_closedisplay(d, 1);
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
		fprint(2, "imageinit: can't open default subfont: %r\n");
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
			fprint(2, "imageinit: can't open default font: %r\n");
			goto Error;
		}
	}else{
		font = openfont(display, fontname);	/* BUG: grey fonts */
		if(font == nil){
			fprint(2, "imageinit: can't open font %s: %r\n", fontname);
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

	snprint(buf, sizeof buf, "%s/winname", display->windir);
	if(gengetwindow(display, buf, &screen, &_screen, ref) < 0)
		goto Error;

	atexit(drawshutdown);

	return 1;
}

int
initdraw(void(*error)(Display*, char*), char *fontname , char *label)
{
	char *dev = "/dev";

	if(access("/dev/draw/new", AEXIST)<0 && bind("#i", "/dev", MAFTER)<0){
		fprint(2, "imageinit: can't bind /dev/draw: %r\n");
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
	Rectangle r;

	fd = open(winname, OREAD);
	if(fd<0 || (n=read(fd, buf, sizeof buf-1))<=0){
		if((image=d->image) == nil){
			fprint(2, "gengetwindow: %r\n");
			*winp = nil;
			d->screenimage = nil;
			return -1;
		}
		strcpy(buf, "noborder");
	}else{
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
			fprint(2, "namedimage %s failed: %r\n", buf);
			*winp = nil;
			d->screenimage = nil;
			return -1;
		}
		assert(image->chan != 0);
	}

	d->screenimage = image;
	*scrp = allocscreen(image, d->white, 0);
	if(*scrp == nil){
		freeimage(d->screenimage);
		*winp = nil;
		d->screenimage = nil;
		return -1;
	}

	r = image->r;
	if(strncmp(buf, "noborder", 8) != 0)
		r = insetrect(image->r, Borderwidth);
	*winp = _allocwindow(*winp, *scrp, r, ref, DWhite);
	if(*winp == nil){
		freescreen(*scrp);
		*scrp = nil;
		freeimage(image);
		d->screenimage = nil;
		return -1;
	}
	d->screenimage = *winp;
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
	char buf[128], info[NINFO+1], *t, isnew;
	int n, datafd, ctlfd, reffd;
	Display *disp;
	Dir *dir;
	Image *image;

	fmtinstall('P', Pfmt);
	fmtinstall('R', Rfmt);
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
	if((n=read(ctlfd, info, sizeof info)) < 12){
    Error2:
		close(ctlfd);
		goto Error1;
	}
	if(n==NINFO+1)
		n = NINFO;
	info[n] = '\0';
	isnew = 0;
	if(n < NINFO)	/* this will do for now, we need something better here */
		isnew = 1;
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
	disp = mallocz(sizeof(Display), 1);
	if(disp == 0){
    Error4:
		close(reffd);
		goto Error3;
	}
	image = nil;
	if(0){
    Error5:
		free(image);
		free(disp);
		goto Error4;
	}
	if(n >= NINFO){
		image = mallocz(sizeof(Image), 1);
		if(image == nil)
			goto Error5;
		image->display = disp;
		image->id = 0;
		image->chan = strtochan(info+2*12);
		image->depth = chantodepth(image->chan);
		image->repl = atoi(info+3*12);
		image->r.min.x = atoi(info+4*12);
		image->r.min.y = atoi(info+5*12);
		image->r.max.x = atoi(info+6*12);
		image->r.max.y = atoi(info+7*12);
		image->clipr.min.x = atoi(info+8*12);
		image->clipr.min.y = atoi(info+9*12);
		image->clipr.max.x = atoi(info+10*12);
		image->clipr.max.y = atoi(info+11*12);
	}

	disp->_isnewdisplay = isnew;
	disp->bufsize = iounit(datafd);
	if(disp->bufsize <= 0)
		disp->bufsize = 8000;
	if(disp->bufsize < 512){
		werrstr("iounit %d too small", disp->bufsize);
		goto Error5;
	}
	disp->buf = malloc(disp->bufsize+5);	/* +5 for flush message */
	if(disp->buf == nil)
		goto Error5;

	disp->image = image;
	disp->dirno = atoi(info+0*12);
	disp->fd = datafd;
	disp->ctlfd = ctlfd;
	disp->reffd = reffd;
	disp->bufp = disp->buf;
	disp->error = error;
	disp->windir = t;
	disp->devdir = strdup(dev);
	qlock(&disp->qlock);
	disp->white = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DWhite);
	disp->black = allocimage(disp, Rect(0, 0, 1, 1), GREY1, 1, DBlack);
	if(disp->white == nil || disp->black == nil){
		free(disp->devdir);
		free(disp->white);
		free(disp->black);
		goto Error5;
	}
	disp->opaque = disp->white;
	disp->transparent = disp->black;
	dir = dirfstat(ctlfd);
	if(dir!=nil && dir->type=='i'){
		disp->local = 1;
		disp->dataqid = dir->qid.path;
	}
	if(dir!=nil && dir->qid.vers==1)	/* other way to tell */
		disp->_isnewdisplay = 1;
	free(dir);

	return disp;
}

/*
 * Call with d unlocked.
 * Note that disp->defaultfont and defaultsubfont are not freed here.
 */
void
closedisplay(Display *disp)
{
	_closedisplay(disp, 0);
}

static void
_closedisplay(Display *disp, int isshutdown)
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

	/*
	 * if we're shutting down, don't free all the resources.
	 * if other procs are getting shot down by notes too,
	 * one might get shot down while holding the malloc lock.
	 * just let the kernel clean things up when we exit.
	 */
	if(isshutdown)
		return;

	free(disp->devdir);
	free(disp->windir);
	freeimage(disp->white);
	freeimage(disp->black);
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
			fprint(1, "proc %d waiting for display lock...\n", getpid());
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

void
drawerror(Display *d, char *s)
{
	char err[ERRMAX];

	if(d && d->error)
		d->error(d, s);
	else{
		errstr(err, sizeof err);
		fprint(2, "draw: %s: %s\n", s, err);
		exits(s);
	}
}

static
int
doflush(Display *d)
{
	int n, nn;

	n = d->bufp-d->buf;
	if(n <= 0)
		return 1;

	if((nn=write(d->fd, d->buf, n)) != n){
		if(_drawdebug)
			fprint(2, "flushimage fail: d=%p: n=%d nn=%d %r\n", d, n, nn); /**/
		d->bufp = d->buf;	/* might as well; chance of continuing */
		return -1;
	}
	d->bufp = d->buf;
	return 1;
}

int
flushimage(Display *d, int visible)
{
	if(d == nil)
		return 0;
	if(visible){
		*d->bufp++ = 'v';	/* five bytes always reserved for this */
		if(d->_isnewdisplay){
			BPLONG(d->bufp, d->screenimage->id);
			d->bufp += 4;
		}
	}
	return doflush(d);
}

uchar*
bufimage(Display *d, int n)
{
	uchar *p;

	if(n<0 || n>d->bufsize){
		werrstr("bad count in bufimage");
		return 0;
	}
	if(d->bufp+n > d->buf+d->bufsize)
		if(doflush(d) < 0)
			return 0;
	p = d->bufp;
	d->bufp += n;
	return p;
}

