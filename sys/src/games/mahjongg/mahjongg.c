#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <stdio.h>
#include "mahjongg.h"

#define MJDIR "/sys/games/lib/mahjongg/"

char *Border	= MJDIR "images/border.bit";
char *Mask	= MJDIR "images/mask.bit";
char *Gameover	= MJDIR "images/gameover.bit";

char *deftileset= MJDIR "tilesets/default.tileset";
char *defbackgr = MJDIR "backgrounds/default.bit";
char *deflayout = MJDIR "layouts/default.layout";

ulong defchan;
int trace;

char *buttons[] =
{
	"deselect",
	"new",
	"restart",
	"resize",
	"exit",
	0
};

Menu menu =
{
	buttons
};

void
usage(void)
{
	fprint(2, "usage: %s [-cf] [-b bg] [-l layout] [-t tileset]\n", argv0);
	exits("usage");
}

Image *
eallocimage(Rectangle r, int repl, uint chan, uint color)
{
	Image *tmp;

	tmp = allocimage(display, r, chan, repl, color);
	if(tmp == nil)
		sysfatal("cannot allocate buffer image: %r");
	return tmp;
}

Image *
eloadfile(char *path)
{
	Image *img;
	int fd;

	fd = open(path, OREAD);
	if(fd < 0) {
		fprint(2, "cannot open image file %s: %r\n", path);
		exits("image");
	}
	img = readimage(display, fd, 0);
	if(img == nil)
		sysfatal("cannot load image: %r");
	close(fd);

	return img;
}

void
allocimages(void)
{
	Rectangle one = Rect(0, 0, 1, 1);

	selected = eallocimage(one, 1, RGBA32, setalpha(DPalebluegreen, 0x5f));
	litbrdr = eallocimage(one, 1, RGBA32, DGreen);
	img = eallocimage(Rect(0, 0, Sizex, Sizey), 0,
		defchan? defchan: screen->chan, DBlack);
	textcol = eallocimage(one, 1, RGBA32, DWhite);

	background = eloadfile(defbackgr);
	replclipr(background, 1, img->r);

	mask = eloadfile(Mask);
	gameover = eloadfile(Gameover);
	tileset = eloadfile(deftileset);
}


void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");
	drawlevel();
}

void
main(int argc, char **argv)
{
	int clickety = 0;
	Mouse m;
	Event e;
	Point origin = Pt(Bord, Bord);

	ARGBEGIN{
	case 'b':
		defbackgr = EARGF(usage());
		break;
	case 'c':
		defchan = RGBA32;
		break;
	case 'f':
		trace = 1;
		break;
	case 'l':
		deflayout = EARGF(usage());
		break;
	case 't':
		deftileset = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc > 0)
		usage();

	if(! parse(deflayout)) {
		fprint(2, "usage: %s [levelfile]\n", argv[0]);
		exits("usage");
	}

	if(initdraw(nil, nil, "mahjongg") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse|Ekeyboard);

	allocimages();

	/* resize to the size of the current level */
	resize(img->r.max);

	generate(time(0));
	drawlevel();
	for(;;) {
		if(level.remaining == 0 && !level.done)
			done();
		switch(event(&e)) {
		case Emouse:
			m = e.mouse;
			if(m.buttons&1) {
				if(level.done)
					break;
				if(!clickety && level.remaining > 0) {
					clickety = 1;
					clicked(subpt(m.xy, addpt(screen->r.min,
						origin)));
				}
			} else {
				clickety = 0;
				if(trace)
					light(subpt(m.xy, addpt(screen->r.min,
						origin)));
			}
			if(m.buttons&2) {
				/* nothing here for the moment */
			}
			if(m.buttons&4)
				switch(emenuhit(3, &m, &menu)) {
				case 0:
					deselect();
					break;
				case 1:
					generate(time(0));
					drawlevel();
					break;
				case 2:
					level = orig;
					drawlevel();
					break;
				case 3:
					resize(img->r.max);
					break;
				case 4:
					exits(nil);
				}
			break;
		case 2:
			switch(e.kbdc) {
			case 127:
			case 'q':
			case 'Q':
				exits(nil);
			case 'h':
			case 'H':
				if(!level.done)
					hint();
				break;
			case 'n':
			case 'N':
				/* new */
				generate(time(0));
				drawlevel();
				break;
			case 'r':
			case 'R':
				level = orig;
				drawlevel();
				break;
			case 'c':
			case 'C':
				if(!level.done) {
					clearlevel();
					done();
				}
				break;
			case 8:
			case 'u':
			case 'U':
				if(level.done) {
					level.done = 0;
					drawlevel();
				}
				undo();
				break;
			}
			break;
		}
	}
}
