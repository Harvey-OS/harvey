#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "sokoban.h"

#define SOKOTREE "/sys/games/lib/sokoban/"

char *LEasy = SOKOTREE "levels/easy.slc";
char *LHard = SOKOTREE "levels/hard.slc";
char *levelfile;

#define SOKOIMG SOKOTREE "images/"

char	*GRImage =	SOKOIMG "right.bit";
char	*GLImage =	SOKOIMG "left.bit";
char	*WallImage =	SOKOIMG "wall.bit";
char	*EmptyImage =	SOKOIMG "empty.bit";
char	*CargoImage =	SOKOIMG "cargo.bit";
char	*GoalCargoImage= SOKOIMG "goalcargo.bit";
char	*GoalImage =	SOKOIMG "goal.bit";
char	*WinImage =	SOKOIMG "win.bit";

char *buttons[] = 
{
	"restart",
	"easy",
	"hard",
	"noanimate", /* this menu string initialized in main */
	"exit",
	0
};

char **levelnames;

Menu menu = 
{
	buttons,
};

Menu lmenu;

void
buildmenu(void)
{
	int i;

	if (levelnames != nil) {
		for(i=0; levelnames[i] != 0; i++)
			free(levelnames[i]);
	}
	levelnames = realloc(levelnames, sizeof(char*)*(numlevels+1));
	if (levelnames == nil)
		sysfatal("cannot allocate levelnames");
	for(i=0; i < numlevels; i++)
		levelnames[i] = genlevels(i);
	levelnames[numlevels] = 0;
	lmenu.item = levelnames;
}

Image *
eallocimage(Rectangle r, int repl, uint color)
{
	Image *tmp;

	tmp = allocimage(display, r, screen->chan, repl, color);
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
	
	bg		= eallocimage(one, 1, DDarkyellow);
	text 		= eallocimage(one, 1, DBluegreen);

	gright = eloadfile(GRImage);
	gleft = eloadfile(GLImage);
	wall = eloadfile(WallImage);
	empty = eloadfile(EmptyImage);
	empty->repl = 1;
	goalcargo = eloadfile(GoalCargoImage);
	cargo = eloadfile(CargoImage);
	goal = eloadfile(GoalImage);
	win = eloadfile(WinImage);
}

int
key2move(int key)
{
	int k = 0;

	switch(key) {
	case 61454:
		k = Up;
		break;
	case 63488:
		k = Down;
		break;
	case 61457:
		k = Left;
		break;
	case 61458:
		k = Right;
		break;
	}

	return k;
}

static Route*
mouse2route(Mouse m)
{
	Point p, q;
	Route *r;

	p = subpt(m.xy, screen->r.min);
	p.x /= BoardX;
	p.y /= BoardY;

	q = subpt(p, level.glenda);
	// fprint(2, "x=%d y=%d\n", q.x, q.y);

	if (q.x == 0 && q.y ==  0)
		return nil;

	if (q.x == 0 || q.y ==  0) {
		if (q.x < 0)
			r = extend(nil, Left, -q.x, Pt(level.glenda.x, p.y));
		else if (q.x > 0)
			r = extend(nil, Right, q.x, Pt(level.glenda.x, p.y));
		else if (q.y < 0)
			r = extend(nil, Up, -q.y, level.glenda);
		else if (q.y > 0)
			r = extend(nil, Down, q.y, level.glenda);
		else
			r = nil;

		if (r != nil && isvalid(level.glenda, r, validpush))
			return r;
		freeroute(r);
	}

	return findroute(level.glenda, p);
}

char *
genlevels(int i)
{
	
	if(i >= numlevels)
		return 0;

	return smprint("level %d", i+1);
}


int
finished(void)
{
	int x, y;
	for(x = 0; x < MazeX; x++)
		for(y = 0; y < MazeY; y++)
			if(level.board[x][y] == Goal)
				return 0;

	return 1;
}

void
eresized(int new)
{
	Point p;

	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");
	
	p = Pt(Dx(screen->r), Dy(screen->r));

	if(!new || !eqpt(p, boardsize(level.max))) {
		drawlevel();
	}
	drawscreen();
}

void 
main(int argc, char **argv)
{
	Mouse m;
	Event ev;
	int e;
	Route *r;
	int timer;
	Animation a;
	int animate;


	if(argc == 2) 
		levelfile = argv[1];
	else
		levelfile = LEasy;
		
	if(! loadlevels(levelfile)) {
		fprint(2, "usage: %s [levelfile]\n", argv[0]);
		exits("usage");
	}
	buildmenu();

	animate = 0;
	buttons[3] = animate ? "noanimate" : "animate";

	if(initdraw(nil, nil, "sokoban") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse|Ekeyboard);

	timer = etimer(0, 200);
	initanimation(&a);

	allocimages();
	glenda = gright;
	eresized(0);

	for(;;) {
		e = event(&ev);
		switch(e) {
		case Emouse:
			m = ev.mouse;
			if(m.buttons&1) {
				stopanimation(&a);
				r = mouse2route(m);
				if (r)
					setupanimation(&a, r);
				if (! animate) {
					while(onestep(&a))
						;
					drawscreen();
				}
			}
			if(m.buttons&2) {
				int l;
				/* levels start from 1 */
				lmenu.lasthit = level.index;
				l=emenuhit(2, &m, &lmenu);
				if(l>=0){
					stopanimation(&a);
					level = levels[l];
					drawlevel();
					drawscreen();
				}
			}
			if(m.buttons&4)
				switch(emenuhit(3, &m, &menu)) {
				case 0:
					stopanimation(&a);
					level = levels[level.index];
					drawlevel();
					drawscreen();
					break;
				case 1:
					stopanimation(&a);
					loadlevels(LEasy);
					buildmenu();
					drawlevel();
					drawscreen();
					break;
				case 2:
					stopanimation(&a);
					loadlevels(LHard);
					buildmenu();
					drawlevel();
					drawscreen();
					break;
				case 3:
					animate = !animate;
					buttons[3] = animate ? "noanimate" : "animate";
					break;
				case 4:
					exits(nil);
				}
			break;

		case Ekeyboard:
			if(level.done)
				break;

			stopanimation(&a);

			switch(ev.kbdc) {
			case 127:
			case 'q':
			case 'Q':
				exits(nil);
			case 'n':
			case 'N':
				if(level.index < numlevels - 1) {
					level = levels[++level.index];
					drawlevel();
					drawscreen();
				}
				break;
			case 'p':
			case 'P':
				if(level.index > 0) {
					level = levels[--level.index];
					drawlevel();
					drawscreen();
				}
				break;
			case 'r':
			case 'R':
				level = levels[level.index];
				drawlevel();
				drawscreen();
				break;
			case 61454:
			case 63488:
			case 61457:
			case 61458:
			case ' ':
				move(key2move(ev.kbdc));
				drawscreen();
				break;
			default:
				// fprint(2, "key: %d]\n", e.kbdc);
				break;
			}
			break;

		default:
			if (e == timer) {
				if (animate)
					onestep(&a);
				else
					while(onestep(&a))
						;
				drawscreen();
			}
			break;
		}

		if(finished()) {
			level.done = 1;
			drawwin();
			drawscreen();
			sleep(3000);
			if(level.index < numlevels - 1) {
				level = levels[++level.index];
				drawlevel();
				drawscreen();
			}
		}
	}
}
