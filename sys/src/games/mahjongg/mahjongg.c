#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "mahjongg.h"

char *Border = "/sys/games/lib/mahjongg/images/border.bit";
char *Mask = "/sys/games/lib/mahjongg/images/mask.bit";
char *Gameover = "/sys/games/lib/mahjongg/images/gameover.bit";

char *deftileset = "/sys/games/lib/mahjongg/tilesets/default.tileset";
char *defbackgr = "/sys/games/lib/mahjongg/backgrounds/default.bit";
char *deflayout = "/sys/games/lib/mahjongg/layouts/default.layout";
ulong defchan;

int trace;


char *buttons[] = 
{
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
usage(char *progname)
{
	fprint(2, "usage: %s [-b background] [-l layout] [-t tileset] [-c] [-f]\n", progname);
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
	img = eallocimage(Rect(0, 0, Sizex, Sizey), 0, defchan ? defchan : screen->chan, DBlack);

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
	Mouse m;
	Event e;
	int clickety = 0;

	ARGBEGIN{
	case 'h':
		usage(argv0);
	case 'f':
		trace = 1;
		break;
	case 'b':
		defbackgr = EARGF(usage(argv0));
		break;
	case 'l':
		deflayout = EARGF(usage(argv0));
		break;
	case 't':
		deftileset = EARGF(usage(argv0));
		break;
	case 'c':
		defchan = RGBA32;
		break;
	}ARGEND

	if(argc > 0) 
		usage(argv0);
		
	if(! parse(deflayout)) {
		fprint(2, "usage: %s [levelfile]\n", argv[0]);
		exits("usage");
	}

	if(initdraw(nil, nil, "mahjongg") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse|Ekeyboard);

	allocimages();
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
					clicked(subpt(m.xy, addpt(screen->r.min, Pt(30, 30))));
				}
			} else {
				clickety = 0;
				if(trace)
					light(subpt(m.xy, addpt(screen->r.min, Pt(30, 30))));
			}
			if(m.buttons&2) {
				/* nothing here for the moment */
			}
			if(m.buttons&4)
				switch(emenuhit(3, &m, &menu)) {
				case 0:
					generate(time(0));
					drawlevel();
					break;
				case 1:
					level = orig;
					drawlevel();
					break;
				case 2:
					resize(img->r.max);
					break;
				case 3:
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
				break;
			case 'r':
			case 'R':
				level = orig;
				break;
			case 'c':
			case 'C':
				clearlevel();
				break;
			}
			if(! level.done)
				drawlevel();
			break;
		}
	}
}
