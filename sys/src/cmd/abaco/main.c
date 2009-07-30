#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>
#include <regexp.h>
#include <plumb.h>
#include <html.h>
#include "dat.h"
#include "fns.h"

enum {
	WPERCOL = 8,
};
void	mousethread(void *);
void	keyboardthread(void *);
void	iconinit(void);
void	plumbproc(void*);

Channel	*cexit;
Channel	*cplumb;
Mousectl *mousectl;

char *fontnames[2] = {
	"/lib/font/bit/lucidasans/unicode.8.font",
	"/lib/font/bit/lucidasans/passwd.6.font",
};

int	snarffd = -1;
int	mainpid;
int	plumbwebfd;
int	plumbsendfd ;
char	*webmountpt = "/mnt/web";
char	*charset = "iso-8859-1";
int	mainstacksize = STACK;

void	readpage(Column *, char *);
int	shutdown(void *, char *);

void
derror(Display *, char *s)
{
	error(s);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-c ncol] [-m mtpt] [-t charset] [url...]\n",
		argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	Column *c;
	char buf[256];
	int i, ncol;

	rfork(RFENVG|RFNAMEG);

	ncol = 1;
	ARGBEGIN{
	case 'c':
		ncol = atoi(EARGF(usage()));
		if(ncol <= 0)
			usage();
		break;
	case 'm':
		webmountpt = EARGF(usage());
		break;
	case 'p':
		procstderr++;
		break;
	case 't':
		charset = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	snprint(buf, sizeof(buf), "%s/ctl", webmountpt);
	webctlfd = open(buf, ORDWR);
	if(webctlfd < 0)
		sysfatal("can't initialize webfs: %r");

	snarffd = open("/dev/snarf", OREAD|OCEXEC);

	if(initdraw(derror, fontnames[0], "abaco") < 0)
		sysfatal("can't open display: %r");
	memimageinit();
	iconinit();
	timerinit();
	initfontpaths();

	cexit = chancreate(sizeof(int), 0);
	crefresh = chancreate(sizeof(Page *), 0);
	if(cexit==nil || crefresh==nil)
		sysfatal("can't create initial channels: %r");

	mousectl = initmouse(nil, screen);
	if(mousectl == nil)
		sysfatal("can't initialize mouse: %r");
	mouse = mousectl;
	keyboardctl = initkeyboard(nil);
	if(keyboardctl == nil)
		sysfatal("can't initialize keyboard: %r");
	mainpid = getpid();
	plumbwebfd = plumbopen("web", OREAD|OCEXEC);
	if(plumbwebfd >= 0){
		cplumb = chancreate(sizeof(Plumbmsg*), 0);
		proccreate(plumbproc, nil, STACK);
	}
	plumbsendfd = plumbopen("send", OWRITE|OCEXEC);

	rowinit(&row, screen->clipr);
	for(i=0; i<ncol; i++){
		c = rowadd(&row, nil, -1);
		if(c==nil && i==0)
			error("initializing columns");
	}
	c = row.col[row.ncol-1];
	for(i=0; i<argc; i++)
		if(i/WPERCOL >= row.ncol)
			readpage(c, argv[i]);
		else
			readpage(row.col[i/WPERCOL], argv[i]);
	flushimage(display, 1);
	threadcreate(keyboardthread, nil, STACK);
	threadcreate(mousethread, nil, STACK);

	threadnotify(shutdown, 1);
	recvul(cexit);
	threadexitsall(nil);
}

void
readpage(Column *c, char *s)
{
	Window *w;
	Runestr rs;

	w = coladd(c, nil, nil, -1);
	bytetorunestr(s, &rs);
	pageget(&w->page, &rs, nil, HGet, TRUE);
	closerunestr(&rs);
}

char *oknotes[] = {
	"delete",
	"hangup",
	"kill",
	"exit",
	nil
};

int
shutdown(void*, char *msg)
{
	int i;

	for(i=0; oknotes[i]; i++)
		if(strncmp(oknotes[i], msg, strlen(oknotes[i])) == 0)
			threadexitsall(msg);
	print("abaco: %s\n", msg);
//	abort();
	return 0;
}

void
plumbproc(void *)
{
	Plumbmsg *m;

	threadsetname("plumbproc");
	for(;;){
		m = plumbrecv(plumbwebfd);
		if(m == nil)
			threadexits(nil);
		sendp(cplumb, m);
	}
}

enum { KTimer, KKey, NKALT, };

void
keyboardthread(void *)
{
	Timer *timer;
	Text *t;
	Rune r;

	static Alt alts[NKALT+1];

	alts[KTimer].c = nil;
	alts[KTimer].v = nil;
	alts[KTimer].op = CHANNOP;
	alts[KKey].c = keyboardctl->c;
	alts[KKey].v = &r;
	alts[KKey].op = CHANRCV;
	alts[NKALT].op = CHANEND;

	timer = nil;
	threadsetname("keyboardthread");
	for(;;){
		switch(alt(alts)){
		case KTimer:
			timerstop(timer);
			alts[KTimer].c = nil;
			alts[KTimer].op = CHANNOP;
			break;
		case KKey:
		casekeyboard:
			typetext = rowwhich(&row, mouse->xy, r, TRUE);
			t = typetext;
			if(t!=nil && t->col!=nil &&
			    !(r==Kdown || r==Kleft || r==Kright))
				/* scrolling doesn't change activecol */
				activecol = t->col;
			if(timer != nil)
				timercancel(timer);
			if(t!=nil){
				texttype(t, r);
				timer = timerstart(500);
				alts[KTimer].c = timer->c;
				alts[KTimer].op = CHANRCV;
			}else{
				timer = nil;
				alts[KTimer].c = nil;
				alts[KTimer].op = CHANNOP;
			}
			if(nbrecv(keyboardctl->c, &r) > 0)
				goto casekeyboard;
			flushimage(display, 1);
			break;
		}
	}
}

void
mousethread(void *)
{
	Plumbmsg *pm;
	Mouse m;
	Text *t;
	int but;
	enum { MResize, MMouse, MPlumb, MRefresh, NMALT };
	static Alt alts[NMALT+1];

	threadsetname("mousethread");
	alts[MResize].c = mousectl->resizec;
	alts[MResize].v = nil;
	alts[MResize].op = CHANRCV;
	alts[MMouse].c = mousectl->c;
	alts[MMouse].v = &mousectl->Mouse;
	alts[MMouse].op = CHANRCV;
	alts[MPlumb].c = cplumb;
	alts[MPlumb].v = &pm;
	alts[MPlumb].op = CHANRCV;
	alts[MRefresh].c = crefresh;
	alts[MRefresh].v = nil;
	alts[MRefresh].op = CHANRCV;
	if(cplumb == nil)
		alts[MPlumb].op = CHANNOP;
	alts[NMALT].op = CHANEND;

	for(;;){
		qlock(&row);
		flushrefresh();
		qunlock(&row);
		flushimage(display, 1);
		switch(alt(alts)){
		case MResize:
			if(getwindow(display, Refnone) < 0)
				error("resized");
			scrlresize();
			tmpresize();
			rowresize(&row, screen->clipr);
			break;
		case MPlumb:
			plumblook(pm);
			plumbfree(pm);
			break;
		case MRefresh:
			break;
		case MMouse:
			m = mousectl->Mouse;
			if(m.buttons == 0)
				continue;

			qlock(&row);
			but = 0;
			if(m.buttons == 1)
				but = 1;
			else if(m.buttons == 2)
				but = 2;
			else if(m.buttons == 4)
				but = 3;

			if(m.buttons & (8|16)){
				if(m.buttons & 8)
					but = Kscrolloneup;
				else
					but = Kscrollonedown;
				rowwhich(&row, m.xy, but, TRUE);
			}else	if(but){
				t = rowwhich(&row, m.xy, but, FALSE);
				if(t)
					textmouse(t, m.xy, but);
			}
			qunlock(&row);
			break;
		}
	}
}

Cursor boxcursor = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00}
};

void
iconinit(void)
{
	Rectangle r;

	/* Green */
	tagcols[BACK] = allocimagemix(display, DPalegreen, DWhite);
	if(tagcols[BACK] == nil)
		error("allocimagemix");
	tagcols[HIGH] = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, DDarkgreen);
	tagcols[BORD] = eallocimage(display, Rect(0,0,1,1), screen->chan, 1, DMedgreen);
	tagcols[TEXT] = display->black;
	tagcols[HTEXT] = display->black;

	/* Grey */
	textcols[BACK] = display->white;
	textcols[HIGH] = eallocimage(display, Rect(0,0,1,1), CMAP8,1, 0xCCCCCCFF);
	textcols[BORD] = display->black;
	textcols[TEXT] = display->black;
	textcols[HTEXT] = display->black;

	r = Rect(0, 0, Scrollsize+2, font->height+1);
	button = eallocimage(display, r, screen->chan, 0, DNofill);
	draw(button, r, tagcols[BACK], nil, r.min);
	r.max.x -= 2;
	border(button, r, 2, tagcols[BORD], ZP);

	r = button->r;
	colbutton = eallocimage(display, r, screen->chan, 0, 0x00994CFF);

	but2col = eallocimage(display, Rect(0,0,1,2), screen->chan, 1, 0xAA0000FF);
	but3col = eallocimage(display, Rect(0,0,1,2), screen->chan, 1, 0x444488FF);

	passfont = openfont(display, fontnames[1]);
	if(passfont == nil)
		error("openfont");
}

/*
 * /dev/snarf updates when the file is closed, so we must open our own
 * fd here rather than use snarffd
 */

/*
 * rio truncates large snarf buffers, so this avoids using the
 * service if the string is huge
 */

enum
{
	NSnarf = 1000,
	MAXSNARF = 100*1024,
};

void
putsnarf(Runestr *rs)
{
	int fd, i, n;

	if(snarffd<0 || rs->nr==0)
		return;
	if(rs->nr > MAXSNARF)
		return;
	fd = open("/dev/snarf", OWRITE);
	if(fd < 0)
		return;
	for(i=0; i<rs->nr; i+=n){
		n = rs->nr-i;
		if(n > NSnarf)
			n =NSnarf;
		if(fprint(fd, "%.*S", n, rs->r) < 0)
			break;
	}
	close(fd);
}

void
getsnarf(Runestr *rs)
{
	int i, n, nb, nulls;
	char *sn, buf[BUFSIZE];

	if(snarffd < 0)
		return;
	sn = nil;
	i = 0;
	seek(snarffd, 0, 0);
	while((n=read(snarffd, buf, sizeof(buf))) > 0){
		sn = erealloc(sn, i+n+1);
		memmove(sn+i, buf, n);
		i += n;
		sn[i] = 0;
	}
	if(i > 0){
		rs->r = runemalloc(i+1);
		cvttorunes(sn, i, rs->r, &nb, &rs->nr, &nulls);
		free(sn);
	}
}
