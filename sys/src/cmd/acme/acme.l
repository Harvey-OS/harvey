#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

void	keyboardtask(chan(int)*, chan(int)*);
void	mousetask(chan(int)*, chan(int)*);
void	waittask(chan(int)*, chan(int)*);
void	xfidalloctask(chan(int)*, chan(int)*);
void	keyboardproc();
void	mouseproc();
void	waitproc(int);
int		mousepid;
int		keyboardpid;
int		timerpid;
int		waitpid;
int		fsyspid;
int		mainpid;
Reffont	**fontcache;
int		nfontcache;
Reffont	*reffonts[2];
byte		*fontnames[2] =
{
	"/lib/font/bit/lucidasans/euro.8.font",
	"/lib/font/bit/lucm/unicode.9.font",
};

Command *command;

int		ALEFrfflag;	/* BUG */
int		ALEFstack;	/* BUG */
chan(int)	*exit0;
chan(int)	*exit1;

void	readfile(Column*, byte*);
void	shutdown(void*, byte*);

void
main(int argc, byte **argv)
{
	int i, ac;
	chan(int) *exitt;
	byte *p, *loadfile;
	byte buf[256];
	Column *c;
	Arg *arg;
	int ncol;

	ncol = -1;

	mainpid = getpid();
	loadfile = nil;
	arg = arginit(argc, argv);
	while(ac = argopt(arg)) switch(ac){
	case 'b':
		bartflag = TRUE;
		break;
	case 'c':
		ncol = atoi(argf(arg));
		break;
	case 'f':
		fontnames[0] = argf(arg);
		break;
	case 'F':
		fontnames[1] = argf(arg);
		break;
	case 'l':
		loadfile = argf(arg);
		break;
	}

	cputype = getenv("cputype");
	objtype = getenv("objtype");
	home = getenv("home");
	rfork(RFENVG|RFNAMEG);
	setenv("font", fontnames[0]);
	bind("/acme/bin", "/bin", MBEFORE);
	if(cputype){
		sprint(buf, "/acme/bin/%s", cputype);
		bind(buf, "/bin", MBEFORE);
	}

	binit(error, fontnames[0], "acme");
	reffont.f = font;
	reffonts[0] = &reffont;
	reffont.inc();	/* one to hold up 'font' variable */
	reffont.inc();	/* one to hold up reffonts[0] */
	fontcache = malloc(sizeof(Reffont*));
	nfontcache = 1;
	fontcache[0] = &reffont;

	ALEFstack = 4000;	/* BUG? */
	iconinit();
	timerinit();
	rxinit();

	alloc cmouse;
	alloc ckeyboard;
	alloc cwait;
	alloc ccommand;
	alloc ckill;
	alloc cxfidalloc;
	alloc	cxfidfree;
	alloc cerr;

	proc keyboardproc();
	proc mouseproc();
	proc waitproc(getpid());

	fsysinit();

	ALEFrfflag &= ~RFNOWAIT;	/* BUG */

	#define	WPERCOL	8
	disk = disk->init();
	if(loadfile)
		row.load(loadfile, TRUE);
	else{
		row.init(screen.clipr);
		if(ncol < 0){
			if(arg->ac == 0)
				ncol = 2;
			else{
				ncol = (arg->ac+(WPERCOL-1))/WPERCOL;
				if(ncol < 2)
					ncol = 2;
			}
		}
		if(ncol == 0)
			ncol = 2;
		for(i=0; i<ncol; i++){
			c = row.add(nil, -1);
			if(c==nil && i==0)
				error("initializing columns");
		}
		c = row.col[row.ncol-1];
		if(arg->ac == 0){
			strcpy(buf, ".");
			getwd(buf, sizeof buf);
			readfile(c, buf);
		}else
			for(i=0; i<arg->ac; i++){
				p = utfrrune(arg->av[i], '/');
				if((p!=nil && strcmp(p, "/guide")==0) || i/WPERCOL>=row.ncol)
					readfile(c, arg->av[i]);
				else
					readfile(row.col[i/WPERCOL], arg->av[i]);
			}
	}
	bflush();

	alloc exit0, exit1;
	alloc *exit0;
	alloc *exit1;
	task keyboardtask(exit1, exit0);
	alloc exitt, *exitt;
	task mousetask(exitt, exit1);
	exit1 = exitt;
	alloc exitt, *exitt;
	task waittask(exitt, exit1);
	exit1 = exitt;
	alloc exitt, *exitt;
	task xfidalloctask(exitt, exit1);
	exit1 = exitt;

	notify(shutdown);
	<-*exit0;
	*exit1 <-= 1;
	killprocs();
	exits(nil);
}

void
readfile(Column *c, byte *s)
{
	Window *w;
	Rune *r, rb[256];
	int nb, nr;

	w = c->add(nil, nil, -1);
	cvttorunes(s, strlen(s), rb, &nb, &nr, nil);
	(r, nr) = cleanname(rb, nr);
	w->setname(r, nr);
	w->body.load(0, s);
	w->body.file->mod = FALSE;
	w->dirty = FALSE;
	w->settag();
	w->body.scrdraw();
	w->tag.setselect(w->tag.file->nc, w->tag.file->nc);
}

byte *oknotes[] ={
	"delete",
	"hangup",
	"kill",
	"exit",
	nil
};

int	dumping;

void
shutdown1(void*, byte *msg)
{
	int i;

	notify(nil);
	if(!dumping && strcmp(msg, "kill")!=0 && strcmp(msg, "exit")!=0 && getpid()==mainpid){
		dumping = TRUE;
		row.dump(nil);
	}
	for(i=0; oknotes[i]; i++)
		if(strncmp(oknotes[i], msg, strlen(oknotes[i])) == 0)
			exits(msg);
	killprocs();
	print("acme: %s\n", msg);
	abort();
}

void
shutdown(void *a, byte *msg)
{
	shutdown1(a, msg);
}

void
killprocs()
{
	Command *c;

	postnote(PNPROC, mousepid, "kill");
	postnote(PNPROC, keyboardpid, "kill");
	postnote(PNPROC, timerpid, "kill");
	postnote(PNPROC, waitpid, "kill");
	postnote(PNPROC, fsyspid, "kill");
	bitblt(&screen, screen.r.min, &screen, screen.r, 0);

	for(c=command; c; c=c->next)
		postnote(PNGROUP, c->pid, "hangup");

}

void
killtasks()
{
	*exit1 <-= 1;
	<-*exit0;
}

uint	keytid;
uint	mousetid;
uint	waittid;
uint	xfidalloctid;

void
keyboardtask(chan(int) *exit0, chan(int) *exit1)
{
	Rune r;
	Timer *timer, null;
	Text *t;

	keytid = ALEF_tid();
	alloc null.c;
	timer = &null;
	typetext = nil;
	for(;;){
		alt{
		case <-*exit0:
			*exit1 <-= 1;
			return;
		case <-(timer->c):
			timerstop(timer);
			t = typetext;
			if(t!=nil && t->what==Tag){
				t->w->lock('K');
				t->w->commit(t);
				t->w->unlock();
				bflush();
			}
			timer = &null;
			break;
		case r = <-ckeyboard:
		casekeyboard:
			typetext = row.type(r, mouse.xy);
			t = typetext;
			if(t!=nil && t->col!=nil)
				activecol = t->col;
			if(t!=nil && t->w!=nil)
				t->w->body.file->curtext = &t->w->body;
			if(timer != &null)
				task timerwaittask(timer);
			if(t!=nil && t->what==Tag)
				timer = timerstart(500);
			else
				timer = &null;
			if(?ckeyboard){
				r = <-ckeyboard;
				goto casekeyboard;
			}
			bflush();
			break;
		}
	}
}

void
mousetask(chan(int) *exit0, chan(int) *exit1)
{
	Text *t, *argt;
	int but, ok;
	uint q0, q1;
	Window *w;

	mouseexit0 = exit0;	/* ugly */
	mouseexit1 = exit1;
	mousetid = ALEF_tid();
	for(;;){
		alt{
		case <-*exit0:
			*exit1 <-= 1;
			return;
		case mouse = <-cmouse:
			row.lock();
			if(mouse.buttons & 0x80){
				screen.r = bscreenrect(&screen.clipr);
				bitblt(&screen, screen.r.min, &screen, screen.r, 0);
				row.reshape(screen.clipr);
				goto Continue;
			}
			t = row.which(mouse.xy);
			if(t!=mousetext && mousetext!=nil && mousetext->w!=nil){
				mousetext->w->lock('M');
				mousetext->eq0 = ~0;
				mousetext->w->commit(mousetext);
				mousetext->w->unlock();
			}
			mousetext = t;
			if(t == nil)
				goto Continue;
			w = t->w;
			if(t==nil || mouse.buttons==0)
				goto Continue;
			if(w)
				w->body.file->curtext = &w->body;
			but = 0;
			if(mouse.buttons == 1)
				but = 1;
			else if(mouse.buttons == 2)
				but = 2;
			else if(mouse.buttons == 4)
				but = 3;
			barttext = t;
			if(t->what==Body && ptinrect(mouse.xy, t->scrollr)){
				if(but){
					w->lock('M');
					t->eq0 = ~0;
					t->scroll(but);
					t->w->unlock();
				}
				goto Continue;
			}
			if(ptinrect(mouse.xy, t->scrollr)){
				if(but){
					if(t->what == Columntag)
						row.dragcol(t->col, but);
					else if(t->what == Tag){
						t->col->dragwin(t->w, but);
						if(t->w)
							barttext = &t->w->body;
					}
					if(t->col)
						activecol = t->col;
				}
				goto Continue;
			}
			if(mouse.buttons){
				if(w)
					w->lock('M');
				t->eq0 = ~0;
				if(w)
					w->commit(t);
				else
					t->commit(TRUE);
				if(mouse.buttons & 1){
					t->select();
					if(w)
						w->settag();
					argtext = t;
					seltext = t;
					if(t->col)
						activecol = t->col;	/* button 1 only */
				}else if(mouse.buttons & 2){
					(ok, argt) = t->select2(&q0, &q1);
					if(ok)
						execute(t, q0, q1, FALSE, argt);
				}else if(mouse.buttons & 4){
					if(t->select3(&q0, &q1))
						look3(t, q0, q1, FALSE);
				}
				if(w)
					w->unlock();
				goto Continue;
			}
    Continue:
			bflush();
			row.unlock();
			break;
		}
	}
}

void
waittask(chan(int) *exit0, chan(int) *exit1)
{
	Waitmsg w;
	Command *c, *lc;
	uint pid;
	int found, ncmd;
	Rune *cmd;
	byte *err;
	Text *t;

	waittid = ALEF_tid();
	command = nil;
	for(;;){
		alt{
		case <-*exit0:
			*exit1 <-= 1;
			return;
		case err = <-cerr:
			row.lock();
			warning(nil, err);
			free(err);
			row.unlock();
			break;
		case cmd = <-ckill:
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
		case w = <-cwait:
			pid = atoi(w.pid);
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
			row.lock();
			t = &row.tag;
			t->commit(TRUE);
			if(c == nil)
				warning(nil, "unknown child pid %d\n", pid);
			else{
				if(search(t, c->name, c->nname)){
					t->delete(t->q0, t->q1, TRUE);
					t->setselect(0, 0);
				}
				if(w.msg[0])
					warning(c->md, "%s\n", w.msg);
				bflush();
			}
			row.unlock();
			if(c){
				free(c->text);
				free(c->av);
				free(c->name);
				fsysdelid(c->md);
				free(c);
			}
			break;
		case c = <-ccommand:
			c->next = command;
			command = c;
			row.lock();
			t = &row.tag;
			t->commit(TRUE);
			t->insert(0, c->name, c->nname, TRUE);
			t->setselect(0, 0);
			row.unlock();
			break;
		}
	}
}

void
xfidalloctask(chan(int) *exit0, chan(int) *exit1)
{
	Xfid *xfree, *x;

	xfidalloctid = ALEF_tid();
	xfree = nil;
	for(;;){
		alt{
		case <-*exit0:
			*exit1 <-= 1;
			return;
		case <-cxfidalloc:
			x = xfree;
			if(x)
				xfree = x->next;
			else{
				x = malloc(sizeof(Xfid));
				alloc x->c;
				task x->ctl();
			}
			cxfidalloc <-= x;
			break;
		case x = <-cxfidfree:
			x->next = xfree;
			xfree = x;
			break;
		}
	}
}

void
frgetmouse()
{
	bflush();
	mouse = <-cmouse;
}

void
keyboardproc()
{
	int fd, m, n;
	byte buf[2*UTFmax];
	Rune r;

	keyboardpid = getpid();
	rfork(RFFDG);
	fd = open("/dev/consctl", OWRITE);
	check fd>=0;
	if(write(fd, "rawon", 5) != 5)
		check 0;
	fd = open("/dev/cons", OREAD);
	check fd >= 0;
	n = 0;
	for(;;){
		while(n>0 && fullrune(buf, n)){
			m = chartorune(&r, buf);
			n -= m;
			memmove(buf, buf+m, n);
			ckeyboard <-= r;
		}
		m = read(fd, buf+n, sizeof buf-n);
		if(m <= 0){
			fprint(2, "kbd: %r\n");
			exits("kbd");
		}
		n += m;
	}
}

void
mouseproc()
{
	int fd, n;
	byte buf[14];
	Mouse m;

	mousepid = getpid();
	rfork(RFFDG);
	fd = open("/dev/mouse", OREAD);
	check fd >= 0;
	for(;;){
		n = read(fd, buf, sizeof buf);
		if(n < sizeof buf){
			fprint(2, "mouse: %r\n");
			exits("mouse");
		}
		m.buttons = buf[1];
		m.xy.x = BGLONG(buf+2);
		m.xy.y = BGLONG(buf+6);
		m.msec = BGLONG(buf+10);
		cmouse <-= m;
	}
}

void
waitproc(int pid)
{
	int fd;
	Waitmsg w;
	byte buf[64];

	waitpid = getpid();
	rfork(RFFDG);
	sprint(buf, "/proc/%d/wait", pid);
	fd = open(buf, OREAD);
	check fd>=0;
	for(;;){
		check read(fd, &w, sizeof(Waitmsg))==sizeof(Waitmsg);
		cwait <-= w;
	}
}

Reffont*
Reffont.get(int fix, int save, int setfont, byte *name)
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
		f = rdfontfile(name, screen.ldepth);
		if(f == nil){
			warning(nil, "can't open font file %s: %r\n", name);
			return nil;
		}
		r = malloc(sizeof(Reffont));
		r->f = f;
		fontcache = realloc(fontcache, (nfontcache+1)*sizeof(Reffont*));
		fontcache[nfontcache++] = r;
	}
    Found:
	if(save){
		r->inc();
		if(reffonts[fix])
			reffonts[fix]->close();
		reffonts[fix] = r;
		fontnames[fix] = name;
	}
	if(setfont){
		reffont.f = r->f;
		r->inc();
		reffonts[0]->close();
		font = r->f;
		reffonts[0] = r;
		r->inc();
		iconinit();
	}
	r->inc();
	return r;
}

void
Reffont.close(Reffont *r)
{
	int i;

	if(r->dec() == 0){
		for(i=0; i<nfontcache; i++)
			if(r == fontcache[i])
				break;
		if(i >= nfontcache)
			warning(nil, "internal error: can't find font in cache\n");
		else{
			nfontcache--;
			memmove(fontcache+i, fontcache+i+1, (nfontcache-i)*sizeof(Reffont*));
		}
		ffree(r->f);
		free(r);
	}
}

byte darkgreybits[] = {
	0xDD, 0xDD, 0x77, 0x77, 0xDD, 0xDD, 0x77, 0x77,
};

byte lightgreybits[] = {
	0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
};

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
iconinit()
{
	Rectangle r;

	if(darkgrey == nil){
		darkgrey = balloc(Rect(0, 0, 16, 4), 0);
		wrbitmap(darkgrey, 0, 4, darkgreybits);
		lightgrey = balloc(Rect(0, 0, 16, 4), 0);
		wrbitmap(lightgrey, 0, 4, lightgreybits);
	}

	if(button){
		bfree(button);
		bfree(modbutton);
		bfree(colbutton);
	}

	r = Rect(0, 0, Scrollwid+1, font->height);
	button = balloc(r, screen.ldepth);
	r = inset(r, 1);
	bitblt(button, r.min, button, r, 0xF);
	r = inset(r, 2);
	bitblt(button, r.min, button, r, 0x0);

	r = button->r;
	modbutton = balloc(r, screen.ldepth);
	r = inset(r, 1);
	bitblt(modbutton, r.min, modbutton, r, 0xF);
	r = inset(r, 2);
	bitblt(modbutton, r.min, modbutton, r, 0x0);
	r = inset(r, 1);
	bitblt(modbutton, r.min, modbutton, r, 0xF);

	r = button->r;
	colbutton = balloc(r, screen.ldepth);
	r = inset(r, 1);
	bitblt(colbutton, r.min, colbutton, r, 0xF);
	r = inset(r, 1);
	texture(colbutton, r, darkgrey, S);
}
