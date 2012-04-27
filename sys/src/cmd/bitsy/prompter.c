#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <control.h>

int	Nline;

enum{
	Back,
	Shade,
	Light,
	Mask,
	Ncol
};

enum {
	Keyback		= 0xeeee9eff,
	Keyshade		= 0xaaaa55ff,
	Keylight		= DWhite,
	Keymask		= 0x0C0C0C0C,
};

Image	*cols[Ncol];

int nline;

char *lines[24];	/* plus one so last line gets terminated by getfields */
Control *entry[24];
Control *kbd;
Control *scrib;
Controlset *keyboard;
Controlset *text;
int kbdy;

int resizeready;

int ctldeletequits = 1;

Channel *eventchan;

void
resizecontrolset(Controlset *cs)
{
	int i;
	Rectangle r, r1;

	if(cs != keyboard)
		return;
	if (!resizeready)
		return;
	if(getwindow(display, Refnone) < 0)
		ctlerror("resize failed: %r");
	draw(screen, screen->r, cols[Back], nil, ZP);
	r = insetrect(screen->r, 4);
	for(i=0; i<Nline; i++){
		r.max.y = r.min.y + font->height;
		ctlprint(entry[i], "rect %R", r);
		ctlprint(entry[i], "show");
		r.min.y = r.max.y;
	}
	kbdy = r.min.y;

	r = screen->r;
	r.min.y = kbdy;
	r.max.y = screen->r.max.y;
	r.min.y = r.max.y - 2*2 - 5*13;
	if(r.min.y >= r.max.y)
		r.min.y = r.max.y;
	r1 = r;
	if(scrib)
		r.max.x = (3*r.max.x + r.min.x)/4;
	ctlprint(kbd, "rect %R", r);
	ctlprint(kbd, "show");
	if(scrib){
		r1.min.x = (3*r1.max.x + r1.min.x)/4;
		ctlprint(scrib, "rect %R", r1);
		ctlprint(scrib, "show");
	}
}

void
readall(char *s)
{
	char *buf;
	int fd;
	Dir *d;

	fd = open(s, OREAD);
	if(fd < 0){
		fprint(2, "prompter: can't open %s: %r\n", s);
		exits("open");
	}
	d = dirfstat(fd);
	if(d == nil){
		fprint(2, "prompter: can't stat %s: %r\n", s);
		exits("stat");
	}
	buf = ctlmalloc(d->length+1);	/* +1 for NUL on end */
	if(read(fd, buf, d->length) != d->length){
		fprint(2, "prompter: can't read %s: %r\n", s);
		exits("stat");
	}
	nline = getfields(buf, lines, nelem(lines), 0, "\n");
	free(d);
	close(fd);
}

void
mousemux(void *v)
{
	Mouse m;
	Channel *c;

	c = v;

	for(;;){
		if(recv(c, &m) < 0)
			break;
		if(m.buttons & 0x20) {
			sendp(eventchan, "mouse: exit");
			break;
		}
		if(m.xy.y >= kbdy)
			send(keyboard->mousec, &m);
		else
			send(text->mousec, &m);
	}
}

void
resizemux(void *v)
{
	Channel *c;

	c = v;

	for(;;){
		if(recv(c, nil) < 0)
			break;
		send(keyboard->resizec, nil);
		send(text->resizec, nil);
	}
}

void
writeall(char *s)
{
	int fd;
	int i, n;

	fd = create(s, OWRITE, 0666);
	if(fd < 0){
		fprint(2, "prompter: can't create %s: %r\n", s);
		exits("open");
	}

	for(n=Nline; --n>=0; )
		if(lines[n][0] != '\0')
			break;

	for(i=0; i<=n; i++)
		fprint(fd, "%s\n", lines[i]);
	close(fd);
}

void
usage(void)
{
	fprint(2, "usage: prompter file\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *s;
	Font *f;
	int i, n;
	char buf[32], *args[3];
	Keyboardctl *kbdctl;
	Mousectl *mousectl;
	Rune r;
	Channel *mtok, *mtot, *ktok, *rtok, *rtot;
	int noscrib;

	noscrib = 0;
	ARGBEGIN{
	case 'n':
		noscrib++;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	readall(argv[0]);

	initdraw(0, 0, "prompter");
	mousectl = initmouse(nil, screen);
	kbdctl = initkeyboard(nil);

	mtok = chancreate(sizeof(Mouse), 0);
	mtot = chancreate(sizeof(Mouse), 0);
	ktok = chancreate(sizeof(Rune), 20);
	rtok = chancreate(sizeof(int), 2);
	rtot = chancreate(sizeof(int), 2);

	initcontrols();

	keyboard = newcontrolset(screen, ktok, mtok, rtok);
	text = newcontrolset(screen, kbdctl->c, mtot, rtot);
	text->clicktotype = 1;

	threadcreate(mousemux, mousectl->c, 4096);
	threadcreate(resizemux, mousectl->resizec, 4096);

	eventchan = chancreate(sizeof(char*), 0);

	cols[Back] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keyback);
	namectlimage(cols[Back], "keyback");
	cols[Light] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keylight);
	namectlimage(cols[Light], "keylight");
	cols[Shade] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, Keyshade);
	namectlimage(cols[Shade], "keyshade");
	cols[Mask] = allocimage(display, Rect(0,0,1,1), RGBA32, 1, Keymask);
	namectlimage(cols[Shade], "keymask");
	f = openfont(display, "/lib/font/bit/lucidasans/boldlatin1.6.font");
	namectlfont(f, "bold");
	f = openfont(display, "/lib/font/bit/lucidasans/unicode.6.font");
	namectlfont(f, "roman");
	font = f;

	Nline = (screen->r.max.y - 2*2 - 5*13 - 8)/font->height;
	if (Nline > nelem(entry)) Nline = nelem(entry);

	for(i=0; i<Nline; i++){
		snprint(buf, sizeof buf, "line.%.2d", i);
		entry[i] = createentry(text, buf);
		ctlprint(entry[i], "font roman");
		ctlprint(entry[i], "image keyback");
		if(i < nline)
			ctlprint(entry[i], "value %q", lines[i]);
		controlwire(entry[i], "event", eventchan);
		activate(entry[i]);
	}

	kbd = createkeyboard(keyboard, "keyboard");
	ctlprint(kbd, "font bold roman");
	ctlprint(kbd, "image keyback");
	ctlprint(kbd, "light keylight");
	ctlprint(kbd, "mask keymask");
	ctlprint(kbd, "border 1");
	controlwire(kbd, "event", eventchan);

	scrib = nil;
	if(!noscrib){
		scrib = createscribble(keyboard, "scribble");
		ctlprint(scrib, "font bold");
		ctlprint(scrib, "image keyback");
		ctlprint(scrib, "border 1");
		controlwire(scrib, "event", eventchan);
		activate(scrib);
	}

	activate(kbd);
	resizeready = 1;
	resizecontrolset(keyboard);

	for(;;){
		s = recvp(eventchan);
		n = tokenize(s, args, nelem(args));
		if(n == 2 && strcmp(args[0], "mouse:")==0 && strcmp(args[1], "exit")==0)
			break;
		if(n == 3)
		if(strcmp(args[0], "keyboard:")==0 || strcmp(args[0], "scribble:")==0)
		if(strcmp(args[1], "value") == 0){
			n = atoi(args[2]);
			if(n == '\033')	/* Escape exits */
				break;
			if(n <= 0xFFFF){
				r = n;
				send(kbdctl->c, &r);
			}
		}
	}

	for(i=0; i<Nline; i++){
		ctlprint(entry[i], "data");
		lines[i] = ctlstrdup(recvp(entry[i]->data));
	}

	writeall(argv[0]);

	draw(screen, screen->r, display->white, nil, ZP);
	flushimage(display, 1);

	threadexitsall(nil);
}
