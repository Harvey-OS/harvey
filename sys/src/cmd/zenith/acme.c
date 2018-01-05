/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"
	/* for generating syms in mkfile only: */
	#include <bio.h>
	#include "edit.h"

void	mousethread(void*);
void	keyboardthread(void*);
void	waitthread(void*);
void	xfidallocthread(void*);
void	newwindowthread(void*);
void plumbproc(void*);

Reffont	**fontcache;
int		nfontcache;
char		wdir[512] = ".";
Reffont	*reffonts[2];
int		snarffd = -1;
int		mainpid;
int		plumbsendfd;
int		plumbeditfd;

enum{
	NSnarf = 1000	/* less than 1024, I/O buffer size */
};
Rune	snarfrune[NSnarf+1];

char		*fontnames[2] =
{
	"/lib/font/bit/fixed/unicode.8x13.font",
	"/lib/font/bit/fixed/unicode.10x20.font"
};

Command *command;

void	acmeerrorinit(void);
void	readfile(Column*, char*);
int	shutdown(void*, char*);

void
derror(Display*d, char *errorstr)
{
	error(errorstr);
}

void
threadmain(int argc, char *argv[])
{
	int i;
	char *p, *loadfile;
	char buf[256];
	char* initscript  = "zenith.init"
;	Column *c;
	int ncol;
	Display *d;

	rfork(RFENVG|RFNAMEG);

	ncol = -1;

	loadfile = nil;
	ARGBEGIN{
	case 'a':
		globalautoindent = TRUE;
		break;
	case 'b':
		bartflag = TRUE;
		break;
	case 'c':
		p = ARGF();
		if(p == nil)
			goto Usage;
		ncol = atoi(p);
		if(ncol <= 0)
			goto Usage;
		break;
	case 'f':
		fontnames[0] = ARGF();
		if(fontnames[0] == nil)
			goto Usage;
		break;
	case 'F':
		fontnames[1] = ARGF();
		if(fontnames[1] == nil)
			goto Usage;
		break;
	case 'l':
		loadfile = ARGF();
		if(loadfile == nil)
			goto Usage;
		break;
	case 'i':
		initscript = ARGF();
		if(initscript == nil)
			goto Usage;
		break;
	default:
	Usage:
		fprint(2, "usage: acme [-ab] [-c ncol] [-f font] [-F fixedfont] [-l loadfile | file...]\n");
		exits("usage");
	}ARGEND

	fontnames[0] = estrdup(fontnames[0]);
	fontnames[1] = estrdup(fontnames[1]);

	quotefmtinstall();
	cputype = getenv("cputype");
	objtype = getenv("objtype");
	home = getenv("home");
	p = getenv("tabstop");
	if(p != nil){
		maxtab = strtoul(p, nil, 0);
		free(p);
	}
	if(maxtab == 0)
		maxtab = 4;
	if(loadfile)
		rowloadfonts(loadfile);
	putenv("font", fontnames[0]);
	snarffd = open("/dev/snarf", OREAD|OCEXEC);
	if(cputype){
		sprint(buf, "/acme/bin/%s", cputype);
		bind(buf, "/bin", MBEFORE);
	}
	bind("/acme/bin", "/bin", MBEFORE);
	getwd(wdir, sizeof wdir);

	if(geninitdraw(nil, derror, fontnames[0], "acme", nil, Refnone) < 0){
		fprint(2, "acme: can't open display: %r\n");
		exits("geninitdraw");
	}
	d = display;
	font = d->defaultfont;

	reffont.f = font;
	reffonts[0] = &reffont;
	incref(&reffont.Ref);	/* one to hold up 'font' variable */
	incref(&reffont.Ref);	/* one to hold up reffonts[0] */
	fontcache = emalloc(sizeof(Reffont*));
	nfontcache = 1;
	fontcache[0] = &reffont;

	iconinit();
	timerinit();
	rxinit();

	cwait = threadwaitchan();
	ccommand = chancreate(sizeof(Command**), 0);
	ckill = chancreate(sizeof(Rune*), 0);
	cxfidalloc = chancreate(sizeof(Xfid*), 0);
	cxfidfree = chancreate(sizeof(Xfid*), 0);
	cnewwindow = chancreate(sizeof(Channel*), 0);
	cerr = chancreate(sizeof(char*), 0);
	cedit = chancreate(sizeof(int), 0);
	cexit = chancreate(sizeof(int), 0);
	cwarn = chancreate(sizeof(void*), 1);
	if(cwait==nil || ccommand==nil || ckill==nil || cxfidalloc==nil || cxfidfree==nil || cerr==nil || cexit==nil || cwarn==nil){
		fprint(2, "acme: can't create initial channels: %r\n");
		exits("channels");
	}

	mousectl = initmouse(nil, screen);
	if(mousectl == nil){
		fprint(2, "acme: can't initialize mouse: %r\n");
		exits("mouse");
	}
	mouse = (Mouse *)mousectl;
	keyboardctl = initkeyboard(nil);
	if(keyboardctl == nil){
		fprint(2, "acme: can't initialize keyboard: %r\n");
		exits("keyboard");
	}
	mainpid = getpid();
	plumbeditfd = plumbopen("edit", OREAD|OCEXEC);
	if(plumbeditfd >= 0){
		cplumb = chancreate(sizeof(Plumbmsg*), 0);
		proccreate(plumbproc, nil, STACK);
	}
	plumbsendfd = plumbopen("send", OWRITE|OCEXEC);

	fsysinit();

	#define	WPERCOL	8
	disk = diskinit();
	if(!loadfile || !rowload(&row, loadfile, TRUE)){
		rowinit(&row, screen->clipr);
		if(ncol < 0){
			if(argc == 0)
				ncol = 2;
			else{
				ncol = (argc+(WPERCOL-1))/WPERCOL;
				if(ncol < 2)
					ncol = 2;
			}
		}
		if(ncol == 0)
			ncol = 2;
		for(i=0; i<ncol; i++){
			c = rowadd(&row, nil, -1);
			if(c==nil && i==0)
				error("initializing columns");
		}
		c = row.col[row.ncol-1];
		if(argc == 0)
			readfile(c, wdir);
		else
			for(i=0; i<argc; i++){
				p = utfrrune(argv[i], '/');
				if((p!=nil && strcmp(p, "/guide")==0) || i/WPERCOL>=row.ncol)
					readfile(c, argv[i]);
				else
					readfile(row.col[i/WPERCOL], argv[i]);
			}
	}
	flushimage(display, 1);

	acmeerrorinit();
	threadcreate(keyboardthread, nil, STACK);
	threadcreate(mousethread, nil, STACK);
	threadcreate(waitthread, nil, STACK);
	threadcreate(xfidallocthread, nil, STACK);
	threadcreate(newwindowthread, nil, STACK);
	loadinitscript("/bin/zenith.init");
	loadinitscript(initscript);

	threadnotify(shutdown, 1);
	recvul(cexit);
	killprocs();
	threadexitsall(nil);
}

void
readfile(Column *c, char *s)
{
	Window *w;
	Rune rb[256];
	int nb, nr;
	Runestr rs;

	w = coladd(c, nil, nil, -1);
	cvttorunes(s, strlen(s), rb, &nb, &nr, nil);
	rs = cleanrname((Runestr){rb, nr});
	winsetname(w, rs.r, rs.nr);
	textload(&w->body, 0, s, 1);
	w->body.file->mod = FALSE;
	w->dirty = FALSE;
	winsettag(w);
	textscrdraw(&w->body);
	textsetselect(&w->tag, w->tag.file->Buffer.nc, w->tag.file->Buffer.nc);
}

char *oknotes[] ={
	"delete",
	"hangup",
	"kill",
	"exit",
	nil
};

int	dumping;

int
shutdown(void*v, char *msg)
{
	int i;

	killprocs();
	if(!dumping && strcmp(msg, "kill")!=0 && strcmp(msg, "exit")!=0 && getpid()==mainpid){
		dumping = TRUE;
		rowdump(&row, nil);
	}
	for(i=0; oknotes[i]; i++)
		if(strncmp(oknotes[i], msg, strlen(oknotes[i])) == 0)
			threadexitsall(msg);
	print("acme: %s\n", msg);
	abort();
	return 0;
}

void
killprocs(void)
{
	Command *c;

	fsysclose();
//	if(display)
//		flushimage(display, 1);

	for(c=command; c; c=c->next)
		postnote(PNGROUP, c->pid, "hangup");
	remove(acmeerrorfile);
}

static int errorfd;

void
acmeerrorproc(void *v)
{
	char *buf;
	int n;

	threadsetname("acmeerrorproc");
	buf = emalloc(8192+1);
	while((n=read(errorfd, buf, 8192)) >= 0){
		buf[n] = '\0';
		sendp(cerr, estrdup(buf));
	}
}

void
acmeerrorinit(void)
{
	int fd, pfd[2];
	char buf[64];

	if(pipe(pfd) < 0)
		error("can't create pipe");
	sprint(acmeerrorfile, "/srv/acme.%s.%d", getuser(), mainpid);
	fd = create(acmeerrorfile, OWRITE, 0666);
	if(fd < 0){
		remove(acmeerrorfile);
  		fd = create(acmeerrorfile, OWRITE, 0666);
		if(fd < 0)
			error("can't create acmeerror file");
	}
	sprint(buf, "%d", pfd[0]);
	write(fd, buf, strlen(buf));
	close(fd);
	/* reopen pfd[1] close on exec */
	sprint(buf, "/fd/%d", pfd[1]);
	errorfd = open(buf, OREAD|OCEXEC);
	if(errorfd < 0)
		error("can't re-open acmeerror file");
	close(pfd[1]);
	close(pfd[0]);
	proccreate(acmeerrorproc, nil, STACK);
}

void
plumbproc(void *v)
{
	Plumbmsg *m;

	threadsetname("plumbproc");
	for(;;){
		m = plumbrecv(plumbeditfd);
		if(m == nil)
			threadexits(nil);
		sendp(cplumb, m);
	}
}

void
keyboardthread(void *v)
{
	Rune r;
	Timer *timer;
	Text *t;
	enum { KTimer, KKey, NKALT };
	static Alt alts[NKALT+1];

	alts[KTimer].c = nil;
	alts[KTimer].v = nil;
	alts[KTimer].op = CHANNOP;
	alts[KKey].c = keyboardctl->c;
	alts[KKey].v = &r;
	alts[KKey].op = CHANRCV;
	alts[NKALT].op = CHANEND;

	timer = nil;
	typetext = nil;
	threadsetname("keyboardthread");
	for(;;){
		switch(alt(alts)){
		case KTimer:
			timerstop(timer);
			t = typetext;
			if(t!=nil && t->what==Tag){
				winlock(t->w, 'K');
				wincommit(t->w, t);
				winunlock(t->w);
				flushimage(display, 1);
			}
			alts[KTimer].c = nil;
			alts[KTimer].op = CHANNOP;
			break;
		case KKey:
		casekeyboard:
			typetext = rowtype(&row, r, mouse->xy);
			t = typetext;
			if(t!=nil && t->col!=nil && !(r==Kdown || r==Kleft || r==Kright))	/* scrolling doesn't change activecol */
				activecol = t->col;
			if(t!=nil && t->w!=nil)
				t->w->body.file->curtext = &t->w->body;
			if(timer != nil)
				timercancel(timer);
			if(t!=nil && t->what==Tag) {
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
mousethread(void *v)
{
	Text *t, *argt;
	int but;
	uint q0, q1;
	Window *w;
	Plumbmsg *pm;
	Mouse m;
	char *act;
	enum { MResize, MMouse, MPlumb, MWarnings, NMALT };
	static Alt alts[NMALT+1];

	threadsetname("mousethread");
	alts[MResize].c = mousectl->resizec;
	alts[MResize].v = nil;
	alts[MResize].op = CHANRCV;
	alts[MMouse].c = mousectl->c;
	alts[MMouse].v = (Mouse *)mousectl;
	alts[MMouse].op = CHANRCV;
	alts[MPlumb].c = cplumb;
	alts[MPlumb].v = &pm;
	alts[MPlumb].op = CHANRCV;
	alts[MWarnings].c = cwarn;
	alts[MWarnings].v = nil;
	alts[MWarnings].op = CHANRCV;
	if(cplumb == nil)
		alts[MPlumb].op = CHANNOP;
	alts[NMALT].op = CHANEND;

	for(;;){
		qlock(&row.QLock);
		flushwarnings();
		qunlock(&row.QLock);
		flushimage(display, 1);
		switch(alt(alts)){
		case MResize:
			if(getwindow(display, Refnone) < 0)
				error("attach to window");
			scrlresize();
			rowresize(&row, screen->clipr);
			break;
		case MPlumb:
			if(strcmp(pm->type, "text") == 0){
				act = plumblookup(pm->attr, "action");
				if(act==nil || strcmp(act, "showfile")==0)
					plumblook(pm);
				else if(strcmp(act, "showdata")==0)
					plumbshow(pm);
			}
			plumbfree(pm);
			break;
		case MWarnings:
			break;
		case MMouse:
			/*
			 * Make a copy so decisions are consistent; mousectl changes
			 * underfoot.  Can't just receive into m because this introduces
			 * another race; see /sys/src/libdraw/mouse.c.
			 */
			m = *(Mouse *)mousectl;
			qlock(&row.QLock);
			t = rowwhich(&row, m.xy);
			if(t!=mousetext && mousetext!=nil && mousetext->w!=nil){
				winlock(mousetext->w, 'M');
				mousetext->eq0 = ~0;
				wincommit(mousetext->w, mousetext);
				winunlock(mousetext->w);
			}
			mousetext = t;
			if(t == nil)
				goto Continue;
			w = t->w;
			if(t==nil || m.buttons==0)
				goto Continue;
			but = 0;
			if(m.buttons == 1)
				but = 1;
			else if(m.buttons == 2)
				but = 2;
			else if(m.buttons == 4)
				but = 3;
			barttext = t;
			if(t->what==Body && ptinrect(m.xy, t->scrollr)){
				if(but){
					winlock(w, 'M');
					t->eq0 = ~0;
					textscroll(t, but);
					winunlock(w);
				}
				goto Continue;
			}
			/* scroll buttons, wheels, etc. */
			if(t->what==Body && w != nil && (m.buttons & (8|16))){
				if(m.buttons & 8)
					but = Kscrolloneup;
				else
					but = Kscrollonedown;
				winlock(w, 'M');
				t->eq0 = ~0;
				texttype(t, but);
				winunlock(w);
				goto Continue;
			}
			if(ptinrect(m.xy, t->scrollr)){
				if(but){
					if(t->what == Columntag)
						rowdragcol(&row, t->col, but);
					else if(t->what == Tag){
						coldragwin(t->col, t->w, but);
						if(t->w)
							barttext = &t->w->body;
					}
					if(t->col)
						activecol = t->col;
				}
				goto Continue;
			}
			if(m.buttons){
				if(w)
					winlock(w, 'M');
				t->eq0 = ~0;
				if(w)
					wincommit(w, t);
				else
					textcommit(t, TRUE);
				if(m.buttons & 1){
					textselect(t);
					if(w)
						winsettag(w);
					argtext = t;
					seltext = t;
					if(t->col)
						activecol = t->col;	/* button 1 only */
					if(t->w!=nil && t==&t->w->body)
						activewin = t->w;
				}else if(m.buttons & 2){
					if(textselect2(t, &q0, &q1, &argt))
						execute(t, q0, q1, FALSE, argt);
				}else if(m.buttons & 4){
					if(textselect3(t, &q0, &q1))
						look3(t, q0, q1, FALSE);
				}
				if(w)
					winunlock(w);
				goto Continue;
			}
    Continue:
			qunlock(&row.QLock);
			break;
		}
	}
}

/*
 * There is a race between process exiting and our finding out it was ever created.
 * This structure keeps a list of processes that have exited we haven't heard of.
 */
typedef struct Pid Pid;
struct Pid
{
	int	pid;
	char	msg[ERRMAX];
	Pid	*next;
};

void
waitthread(void *v)
{
	Waitmsg *w;
	Command *c, *lc;
	uint pid;
	int found, ncmd;
	Rune *cmd;
	char *err;
	Text *t;
	Pid *pids, *p, *lastp;
	enum { WErr, WKill, WWait, WCmd, NWALT };
	Alt alts[NWALT+1];

	threadsetname("waitthread");
	pids = nil;
	alts[WErr].c = cerr;
	alts[WErr].v = &err;
	alts[WErr].op = CHANRCV;
	alts[WKill].c = ckill;
	alts[WKill].v = &cmd;
	alts[WKill].op = CHANRCV;
	alts[WWait].c = cwait;
	alts[WWait].v = &w;
	alts[WWait].op = CHANRCV;
	alts[WCmd].c = ccommand;
	alts[WCmd].v = &c;
	alts[WCmd].op = CHANRCV;
	alts[NWALT].op = CHANEND;

	command = nil;
	for(;;){
		switch(alt(alts)){
		case WErr:
			qlock(&row.QLock);
			warning(nil, "%s", err);
			free(err);
			flushimage(display, 1);
			qunlock(&row.QLock);
			break;
		case WKill:
			found = FALSE;
			ncmd = runestrlen(cmd);
			for(c=command; c; c=c->next){
				/* -1 for blank */
				if(runeeq(c->name, c->nname-1, cmd, ncmd) == TRUE){
					if(postnote(PNGROUP, c->pid, "kill") < 0)
						warning(nil, "kill %S: %r\n", cmd);
					found = TRUE;
				}
			}
			if(!found)
				warning(nil, "Kill: no process %S\n", cmd);
			free(cmd);
			break;
		case WWait:
			pid = w->pid;
			lc = nil;
			for(c=command; c; c=c->next){
				if(c->pid == pid){
					if(lc)
						lc->next = c->next;
					else
						command = c->next;
					break;
				}
				lc = c;
			}
			qlock(&row.QLock);
			t = &row.tag;
			textcommit(t, TRUE);
			if(c == nil){
				/* helper processes use this exit status */
				if(strncmp(w->msg, "libthread", 9) != 0){
					p = emalloc(sizeof(Pid));
					p->pid = pid;
					strncpy(p->msg, w->msg, sizeof(p->msg));
					p->next = pids;
					pids = p;
				}
			}else{
				if(search(t, c->name, c->nname)){
					textdelete(t, t->q0, t->q1, TRUE);
					textsetselect(t, 0, 0);
				}
				if(w->msg[0])
					warning(c->md, "%s\n", w->msg);
				flushimage(display, 1);
			}
			qunlock(&row.QLock);
			free(w);
    Freecmd:
			if(c){
				if(c->iseditcmd)
					sendul(cedit, 0);
				free(c->text);
				free(c->name);
				fsysdelid(c->md);
				free(c);
			}
			break;
		case WCmd:
			/* has this command already exited? */
			lastp = nil;
			for(p=pids; p!=nil; p=p->next){
				if(p->pid == c->pid){
					if(p->msg[0])
						warning(c->md, "%s\n", p->msg);
					if(lastp == nil)
						pids = p->next;
					else
						lastp->next = p->next;
					free(p);
					goto Freecmd;
				}
				lastp = p;
			}
			c->next = command;
			command = c;
			qlock(&row.QLock);
			t = &row.tag;
			textcommit(t, TRUE);
			textinsert(t, 0, c->name, c->nname, TRUE);
			textsetselect(t, 0, 0);
			flushimage(display, 1);
			qunlock(&row.QLock);
			break;
		}
	}
}

void
xfidallocthread(void*v)
{
	Xfid *xfree, *x;
	enum { Alloc, Free, N };
	static Alt alts[N+1];

	threadsetname("xfidallocthread");
	alts[Alloc].c = cxfidalloc;
	alts[Alloc].v = nil;
	alts[Alloc].op = CHANRCV;
	alts[Free].c = cxfidfree;
	alts[Free].v = &x;
	alts[Free].op = CHANRCV;
	alts[N].op = CHANEND;

	xfree = nil;
	for(;;){
		switch(alt(alts)){
		case Alloc:
			x = xfree;
			if(x)
				xfree = x->next;
			else{
				x = emalloc(sizeof(Xfid));
				x->c = chancreate(sizeof(void(*)(Xfid*)), 0);
				x->arg = x;
				threadcreate(xfidctl, x->arg, STACK);
			}
			sendp(cxfidalloc, x);
			break;
		case Free:
			x->next = xfree;
			xfree = x;
			break;
		}
	}
}

/* this thread, in the main proc, allows fsysproc to get a window made without doing graphics */
void
newwindowthread(void*v)
{
	Window *w;

	threadsetname("newwindowthread");

	for(;;){
		/* only fsysproc is talking to us, so synchronization is trivial */
		recvp(cnewwindow);
		w = makenewwindow(nil);
		winsettag(w);
		sendp(cnewwindow, w);
	}
}

Reffont*
rfget(int fix, int save, int setfont, char *name)
{
	Reffont *r;
	Font *f;
	int i;

	r = nil;
	if(name == nil){
		name = fontnames[fix];
		r = reffonts[fix];
	}
	if(r == nil){
		for(i=0; i<nfontcache; i++)
			if(strcmp(name, fontcache[i]->f->name) == 0){
				r = fontcache[i];
				goto Found;
			}
		f = openfont(display, name);
		if(f == nil){
			warning(nil, "can't open font file %s: %r\n", name);
			return nil;
		}
		r = emalloc(sizeof(Reffont));
		r->f = f;
		fontcache = erealloc(fontcache, (nfontcache+1)*sizeof(Reffont*));
		fontcache[nfontcache++] = r;
	}
    Found:
	if(save){
		incref(&r->Ref);
		if(reffonts[fix])
			rfclose(reffonts[fix]);
		reffonts[fix] = r;
		if(name != fontnames[fix]){
			free(fontnames[fix]);
			fontnames[fix] = estrdup(name);
		}
	}
	if(setfont){
		reffont.f = r->f;
		incref(&r->Ref);
		rfclose(reffonts[0]);
		font = r->f;
		reffonts[0] = r;
		incref(&r->Ref);
		iconinit();
	}
	incref(&r->Ref);
	return r;
}

void
rfclose(Reffont *r)
{
	int i;

	if(decref(&r->Ref) == 0){
		for(i=0; i<nfontcache; i++)
			if(r == fontcache[i])
				break;
		if(i >= nfontcache)
			warning(nil, "internal error: can't find font in cache\n");
		else{
			nfontcache--;
			memmove(fontcache+i, fontcache+i+1, (nfontcache-i)*sizeof(Reffont*));
		}
		freefont(r->f);
		free(r);
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
	Image *tmp;

	/* Blue */
	tagcols[BACK] = allocimagemix(display, DPalebluegreen, DWhite);
	tagcols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DPalegreygreen);
	tagcols[BORD] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DPurpleblue);
	tagcols[TEXT] = display->black;
	tagcols[HTEXT] = display->black;

	/* Yellow */
	textcols[BACK] = allocimagemix(display, DPaleyellow, DWhite);
	textcols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DDarkyellow);
	textcols[BORD] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DYellowgreen);
	textcols[TEXT] = display->black;
	textcols[HTEXT] = display->black;

	if(button){
		freeimage(button);
		freeimage(modbutton);
		freeimage(colbutton);
	}

	r = Rect(0, 0, Scrollwid+2, font->height+1);
	button = allocimage(display, r, screen->chan, 0, DNofill);
	draw(button, r, tagcols[BACK], nil, r.min);
	r.max.x -= 2;
	border(button, r, 2, tagcols[BORD], ZP);

	r = button->r;
	modbutton = allocimage(display, r, screen->chan, 0, DNofill);
	draw(modbutton, r, tagcols[BACK], nil, r.min);
	r.max.x -= 2;
	border(modbutton, r, 2, tagcols[BORD], ZP);
	r = insetrect(r, 2);
	tmp = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DMedblue);
	draw(modbutton, r, tmp, nil, ZP);
	freeimage(tmp);

	r = button->r;
	colbutton = allocimage(display, r, screen->chan, 0, DPurpleblue);

	but2col = allocimage(display, r, screen->chan, 1, 0xAA0000FF);
	but3col = allocimage(display, r, screen->chan, 1, 0x006600FF);
}

/*
 * /dev/snarf updates when the file is closed, so we must open our own
 * fd here rather than use snarffd
 */

/* rio truncates larges snarf buffers, so this avoids using the
 * service if the string is huge */

#define MAXSNARF 100*1024

void
putsnarf(void)
{
	int fd, i, n;

	if(snarffd<0 || snarfbuf.nc==0)
		return;
	if(snarfbuf.nc > MAXSNARF)
		return;
	fd = open("/dev/snarf", OWRITE);
	if(fd < 0)
		return;
	for(i=0; i<snarfbuf.nc; i+=n){
		n = snarfbuf.nc-i;
		if(n >= NSnarf)
			n = NSnarf;
		bufread(&snarfbuf, i, snarfrune, n);
		if(fprint(fd, "%.*S", n, snarfrune) < 0)
			break;
	}
	close(fd);
}

void
getsnarf()
{
	int nulls;

	if(snarfbuf.nc > MAXSNARF)
		return;
	if(snarffd < 0)
		return;
	seek(snarffd, 0, 0);
	bufreset(&snarfbuf);
	bufload(&snarfbuf, 0, snarffd, &nulls);
}
