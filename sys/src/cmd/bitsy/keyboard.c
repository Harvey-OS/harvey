#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>
#include <scribble.h>

int debug;
int kbdonly;

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

Image	*cols[Ncol];

Control *kbd;
Control *scrib;
Controlset *keyboard;

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
		if ((m.buttons & 0x24) == 0) {
			if (ob & 0x24) {
				/* hide button just came up */
				if (hidden()) {
					if (debug) threadprint(2, "unhide");
					if (threadprint(wctl, "unhide") <= 0)
						threadprint(2, "unhide failed: %r\n");
				} else {
					if (debug) threadprint(2, "hide");
					if (threadprint(wctl, "hide") <= 0)
						threadprint(2, "hide failed: %r\n");
				}
			} else
				send(keyboard->mousec, &m);
		}
	}
}

void
resizecontrolset(Controlset*)
{
	Rectangle r, rk, rs;

	if(getwindow(display, Refnone) < 0)
		ctlerror("resize failed: %r");

	if (hidden())
		return;

	r = display->image->r;

	if (eqrect(r, screen->r) == 0) {
		r = Rect(r.min.x, r.max.y - kbdheight - 2*Borderwidth, r.max.x, r.max.y);
		if (threadprint(wctl, "resize -r %d %d %d %d", r.min.x, r.min.y, r.max.x, r.max.y) <= 0) {
			return;
		}
		if (getwindow(display, Refbackup) < 0)
			sysfatal("getwindow");
		if (debug) threadprint(2, "resize: %R\n", screen->r);
	}

	if (kbdonly) {
		printctl(kbd->ctl, "rect %R\nshow", screen->r);
	} else {
		rk = screen->r;
		rs = screen->r;
		rk.max.x = (3*rk.max.x + rk.min.x)/4;
		printctl(kbd->ctl, "rect %R\nshow", rk);
		rs.min.x = (3*rs.max.x + rs.min.x)/4;
		printctl(scrib->ctl, "rect %R\nshow", rs);
	}
}

void
usage(void)
{
	threadprint(2, "usage: keyboard\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *s;
	Font *f;
	int i, n, kbdfd;
	char *args[3], str[UTFmax+1];
	Channel *c;
	Rune r;
	Mousectl	*mousectl;
	Channel *mtok;

	ARGBEGIN{
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
		if (debug) threadprint(2, "open %s: %r\n", "#r/kbdin");
		kbdfd = 1;
	}

	initdraw(0, 0, "keyboard");
	mousectl = initmouse(nil, screen);

	wctl = open("/dev/wctl", ORDWR);
	if (wctl < 0) {
		threadprint(2, "open %s: %r\n", "/dev/wctl");
		wctl = 2;	/* for debugging */
	}

	mtok = chancreate(sizeof(Mouse), 0);

	initcontrols();

	keyboard = newcontrolset(screen, nil, mtok, mousectl->resizec);

	threadcreate(mousemux, mousectl->c, 4096);

	c = chancreate(sizeof(char*), 0);

	cols[Back] = allocimage(display, Rect(0,0,1,1), display->chan, 1, Keyback);
	namectlimage(cols[Back], "keyback");
	cols[Light] = allocimage(display, Rect(0,0,1,1), display->chan, 1, Keylight);
	namectlimage(cols[Light], "keylight");
	cols[Shade] = allocimage(display, Rect(0,0,1,1), display->chan, 1, Keyshade);
	namectlimage(cols[Shade], "keyshade");
	cols[Mask] = allocimage(display, Rect(0,0,1,1), RGBA32, 1, Keymask);
	namectlimage(cols[Shade], "keymask");

	kbd = createkeyboard(keyboard, "keyboard");
	f = openfont(display, "/lib/font/bit/lucidasans/boldlatin1.6.font");
	namectlfont(f, "keyfont");
	f = openfont(display, "/lib/font/bit/lucidasans/unicode.6.font");
	namectlfont(f, "keyctlfont");
	printctl(kbd->ctl, "font keyfont keyctlfont");
	printctl(kbd->ctl, "image keyback");
	printctl(kbd->ctl, "light keylight");
	printctl(kbd->ctl, "mask keymask");
	printctl(kbd->ctl, "border 1");
	controlwire(kbd, "event", c);
	activate(kbd);

	if (kbdonly == 0) {
		scrib = createscribble(keyboard, "scribble");
		printctl(scrib->ctl, "font keyfont");
		printctl(scrib->ctl, "image keyback");
		printctl(scrib->ctl, "border 1");
		controlwire(scrib, "event", c);
		activate(scrib);
	}

	resizecontrolset(nil);

	for(;;){
		s = recvp(c);
		n = tokenize(s, args, nelem(args));
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
	}
	threadexitsall(nil);
}
