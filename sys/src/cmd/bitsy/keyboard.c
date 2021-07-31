#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <scribble.h>

int debug = 0;
typedef struct Win Win;
struct Win {
	int n;
	int dirty;
	char *label;
	Control *button;
};

Win *win;
int nwin;
int mwin;
int onwin;
int rows, cols;
int kbdonly;
int scribbleleft;
int winshow;

Channel *kc;
Channel *ec;
Channel *tc;
Rectangle r, rk, rs, rw;
Font *keyfont, *keyctlfont;

enum{
	Back,
	Shade,
	Light,
	Mask,
	Ncol
};

enum {
	kbdheight = 2 + 5*13,
};

enum {
	Keyback		= 0xeeee9eff,
	Keyshade		= 0xaaaa55ff,
	Keylight		= DWhite,
	Keymask		= 0x0C0C0C0C,
};

Image	*colors[Ncol];

Control *kbd;
Control *scrib;
Control *boxbox;
Controlset *cs;

int	ctldeletequits = 1;
int	wctl;

int
hidden(void)
{
	char buf[128];
	int n;

	close(wctl);
	if ((wctl = open("/dev/wctl", ORDWR)) < 0)
		return 0;
	n = read(wctl, buf, sizeof buf-1);
	if (n <= 0)
		sysfatal("wctl read: %r");
	buf[n] = 0;
	return strstr(buf, "visible") == nil;
}

void
mousemux(void *v)
{
	Mouse m;
	Channel *c;
	int ob;

	c = v;

	for(ob = 0;;ob = m.buttons){
		if(recv(c, &m) < 0)
			break;
		if ((m.buttons & 0x20) == 0) {
			if (ob & 0x20) {
				/* hide button just came up */
				if (hidden()) {
					if (debug) fprint(2, "unhide");
					if (fprint(wctl, "unhide") <= 0)
						fprint(2, "unhide failed: %r\n");
				} else {
					if (debug) fprint(2, "hide");
					if (fprint(wctl, "hide") <= 0)
						fprint(2, "hide failed: %r\n");
				}
			} else
				send(cs->mousec, &m);
		}
	}
}

void
refreshwin(void)
{
	char label[128];
	int i, fd, lfd, n, nr, nw, m;
	Dir *pd;

	if((fd = open("/dev/wsys", OREAD)) < 0)
		return;

	nw = 0;
/* i'd rather read one at a time but rio won't let me */
	while((nr=dirread(fd, &pd)) > 0){
		for(i=0; i<nr; i++){
			n = atoi(pd[i].name);
			sprint(label, "/dev/wsys/%d/label", n);
			if((lfd = open(label, OREAD)) < 0)
				continue;
			m = read(lfd, label, sizeof(label)-1);
			close(lfd);
			if(m < 0)
				continue;
			label[m] = '\0';
			if(nw < nwin && win[nw].n == n && strcmp(win[nw].label, label)==0){
				nw++;
				continue;
			}

			if(nw < nwin){
				free(win[nw].label);
				win[nw].label = nil;
			}
			if(nw >= mwin){
				mwin += 8;
				win = ctlrealloc(win, mwin*sizeof(win[0]));
				memset(&win[mwin-8], 0, 8*sizeof(win[0]));
			}
			win[nw].n = n;
			win[nw].label = ctlstrdup(label);
			win[nw].dirty = 1;
			sprint(label, "%d", nw);
			if (win[nw].button == nil){
				win[nw].button = createtextbutton(cs, label);
				chanprint(cs->ctl, "%q font keyfont", label);
				chanprint(cs->ctl, "%q image keyback", label);
				chanprint(cs->ctl, "%q pressedtextcolor red", label);
				chanprint(cs->ctl, "%q mask transparent", label);
				chanprint(cs->ctl, "%q border 1", label);
				chanprint(cs->ctl, "%q bordercolor black", label);
				chanprint(cs->ctl, "%q align centerleft", label);
				chanprint(cs->ctl, "%q size 16 %d 512 %d", label, keyfont->height+2, keyfont->height+2);
				controlwire(win[nw].button, "event", ec);
			}
			if (nw >= nwin){
				activate(win[nw].button);
				chanprint(cs->ctl, "cols add %q", label);
			}
			chanprint(cs->ctl, "%q text %q", win[nw].button->name, win[nw].label);
			nw++;
		}
	}
	for(i = nw; i < nwin; i++){
		free(win[i].label);
		win[i].label = nil;
		deactivate(win[i].button);
		chanprint(cs->ctl, "cols remove %q", win[i].button->name);
	}
	nwin = nw;
	close(fd);
	if (rw.max.x)
		chanprint(cs->ctl, "cols rect %R\ncols show", rw);
}

void
resizecontrolset(Controlset*)
{
	int fd;
	char buf[61];


	if(getwindow(display, Refnone) < 0)
		ctlerror("resize failed: %r");

	if (hidden()) {
		if (debug) fprint(2, "resizecontrolset: hidden\n");
		return;
	}

	fd = open("/dev/screen", OREAD);
	if (fd < 0) {
		r = display->image->r;
		if (debug) fprint(2, "display->imgae->r: %R\n", r);
	} else {
		if (read(fd, buf, 60) != 60)
			sysfatal("resizecontrolset: read: /dev/screen: %r");
		close(fd);
		buf[60] = '\0';
		r.min.x = atoi(buf+1+1*12);
		r.min.y = atoi(buf+1+2*12);
		r.max.x = atoi(buf+1+3*12);
		r.max.y = atoi(buf+1+4*12);
		if (debug) fprint(2, "/dev/screen: %R\n", r);
	}
	r = insetrect(r, 4);
	r.min.y = r.max.y - kbdheight - 2*Borderwidth;
	if (debug) fprint(2, "before eqrect: %R\n", r);
	if (eqrect(r, screen->r) == 0) {
		if (debug) fprint(2, "resizecontrolset: resize %R\n", r);
		if (fprint(wctl, "resize -r %R", insetrect(r, -4)) <= 0) {
			fprint(2, "resizecontrolset: resize failed\n");
		}
		return;
	}

	if (debug) fprint(2, "after eqrect: %R\n", r);
	rk = r;
	if (winshow){
		rw = rk;
		rw.min.x = (3*rk.max.x + rk.min.x)/4;
		rk.max.x = rw.min.x;
		if (debug) fprint(2, "rw: rect %R\n", rw);
		chanprint(cs->ctl, "cols rect %R\ncols show", rw);
	}
	if (kbdonly) {
		chanprint(cs->ctl, "keyboard rect %R\nkeyboard show", rk);
	} else {
		rs = rk;
		if (scribbleleft){
			rk.min.x = (rk.max.x + 3*rk.min.x)/4;
			rs.max.x = rk.min.x;
		}else{
			rk.max.x = (3*rk.max.x + rk.min.x)/4;
			rs.min.x = rk.max.x;
		}
		chanprint(cs->ctl, "keyboard rect %R\nkeyboard show", rk);
		if (debug) fprint(2, "rk: rect %R\nkeyboard show\n", rk);
		chanprint(cs->ctl, "scribble rect %R\nscribble show", rs);
		if (debug) fprint(2, "rs: rect %R\nscribble show\n", rs);
	}
}

void
usage(void)
{
	fprint(2, "usage: keyboard\n");
	threadexitsall("usage");
}

void
timerproc(void*v)
{
	Channel *c;

	c = v;
	for(;;){
		sleep(5000);
		sendul(c, 1);
	}
}

void
watchproc(void*)
{

	for(;;){}
}

void
threadmain(int argc, char *argv[])
{
	int i, n, kbdfd;
	char str[UTFmax+1];
	Rune r;
	Mousectl	*mousectl;
	Channel *mtok;
	char *e, buf[128], *args[8];
	int fd;

	ARGBEGIN{
	case 'w':
		winshow++;
		break;
	case 'l':
		scribbleleft++;
		break;
	case 'n':
		kbdonly++;
		break;
	case 'd':
		ScribbleDebug++;
		debug++;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 0)
		usage();

	kbdfd = open("/dev/kbdin", OWRITE);
	if (kbdfd < 0 && (kbdfd = open("#r/kbdin", OWRITE)) < 0) {
		if (debug) fprint(2, "open %s: %r\n", "#r/kbdin");
		kbdfd = 1;
	}

	initdraw(0, 0, "keyboard");
	mousectl = initmouse(nil, screen);

	wctl = open("/dev/wctl", ORDWR);
	if (wctl < 0) {
		fprint(2, "open %s: %r\n", "/dev/wctl");
		wctl = 2;	/* for debugging */
	}

	mtok = chancreate(sizeof(Mouse), 0);

	initcontrols();

	cs = newcontrolset(screen, nil, mtok, mousectl->resizec);

	threadcreate(mousemux, mousectl->c, 4096);

	kc = chancreate(sizeof(char*), 0);
	tc = chancreate(sizeof(int), 1);
	ec = chancreate(sizeof(char*), 1);

	colors[Back] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keyback);
	namectlimage(colors[Back], "keyback");
	colors[Light] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keylight);
	namectlimage(colors[Light], "keylight");
	colors[Shade] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keyshade);
	namectlimage(colors[Shade], "keyshade");
	colors[Mask] = allocimage(display, Rect(0,0,1,1), RGBA32, 1, Keymask);
	namectlimage(colors[Shade], "keymask");
	keyfont = openfont(display, "/lib/font/bit/lucidasans/boldlatin1.6.font");
	namectlfont(keyfont, "keyfont");
	keyctlfont = openfont(display, "/lib/font/bit/lucidasans/unicode.6.font");
	namectlfont(keyctlfont, "keyctlfont");

	kbd = createkeyboard(cs, "keyboard");
	chanprint(cs->ctl, "keyboard font keyfont keyctlfont");
	chanprint(cs->ctl, "keyboard image keyback");
	chanprint(cs->ctl, "keyboard light keylight");
	chanprint(cs->ctl, "keyboard mask keymask");
	chanprint(cs->ctl, "keyboard border 1");
	chanprint(cs->ctl, "keyboard size %d %d %d %d", 246, 2 + 5 * (keyfont->height + 1), 512, 256);
	controlwire(kbd, "event", kc);

	if (kbdonly == 0){
		scrib = createscribble(cs, "scribble");
		if (scrib == nil)
			sysfatal("createscribble");
		chanprint(cs->ctl, "scribble font keyfont");
		chanprint(cs->ctl, "scribble image keyback");
		chanprint(cs->ctl, "scribble border 1");
		controlwire(scrib, "event", kc);
	}

	if (winshow){
		boxbox = createboxbox(cs, "cols");
	
		chanprint(cs->ctl, "cols border 2");
		chanprint(cs->ctl, "cols bordercolor keyback");
	}

	resizecontrolset(nil);

	activate(kbd);
	if (kbdonly == 0)
		activate(scrib);
	if (winshow){
		refreshwin();
		proccreate(timerproc, tc, 2048);
	}

	for(;;){
		Alt a[] = {
			{ kc, &e, CHANRCV },
			{ ec, &e, CHANRCV },
			{ tc, &n, CHANRCV },
			{ nil, nil, CHANEND }
		};
		switch(alt(a)){
		case 0:	/* Keyboard */
			n = tokenize(e, args, nelem(args));
			if(n == 3)
			if(strcmp(args[0], "keyboard:")==0 || strcmp(args[0], "scribble:")==0)
			if(strcmp(args[1], "value") == 0){
				n = atoi(args[2]);
				if(n <= 0xFFFF){
					r = n;
					i = runetochar(str, &r);
					write(kbdfd, str, i);
				}
			}
			break;
		case 1:	/* Button event */
			n = tokenize(e, args, nelem(args));
			if (n != 3 || strcmp(args[1], "value"))
				sysfatal("event string");
			i = atoi(args[0]);
			if (i < 0 || i >= nwin)
				sysfatal("win out of range: %d of %d", i, nwin);
			n = atoi(args[2]);
			if (n){
				sprint(buf, "/dev/wsys/%d/wctl", win[i].n);
				if((fd = open(buf, OWRITE)) >= 0){
					while (write(fd, "top\n", 4) < 0) {
						/* wait until mouse comes up */
						rerrstr(buf, sizeof buf);
						if (strncmp(buf, "action disallowed when mouse active", sizeof buf)){
							fprint(2, "write top: %s\n", buf);
							break;
						}
						sleep(100);
					}
					if (write(fd, "current\n", 8) < 0)
						fprint(2, "write current: %r\n");
					close(fd);
				}
				chanprint(cs->ctl, "%q value 0", win[i].button->name);
			}
			break;
		case 2:
			refreshwin();
			break;
		}
	}
}
