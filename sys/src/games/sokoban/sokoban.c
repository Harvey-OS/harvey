#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "sokoban.h"

char *LEasy = "/sys/games/lib/sokoban/levels/easy.slc";
char *LHard = "/sys/games/lib/sokoban/levels/hard.slc";
char *levelfile;

char		*GRImage = "/sys/games/lib/sokoban/images/right.bit";
char		*GLImage = "/sys/games/lib/sokoban/images/left.bit";
char		*WallImage = "/sys/games/lib/sokoban/images/wall.bit";
char		*EmptyImage = "/sys/games/lib/sokoban/images/empty.bit";
char		*CargoImage = "/sys/games/lib/sokoban/images/cargo.bit";
char		*GoalCargoImage = "/sys/games/lib/sokoban/images/goalcargo.bit";
char		*GoalImage = "/sys/games/lib/sokoban/images/goal.bit";
char		*WinImage = "/sys/games/lib/sokoban/images/win.bit";


char *buttons[] = 
{
	"restart",
	"easy",
	"hard",
	"exit",
	0
};

Menu menu = 
{
	buttons
};

Menu lmenu =
{
	nil,
	genlevels,
	0,
};

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
int
mousemove(Mouse m)
{
	Point p;

	p = subpt(m.xy, screen->r.min);
	p.x /= BoardX;
	p.y /= BoardY;

	if(eqpt(p, addpt(level.glenda, Pt(0, -1))))
		return Up;
	if(eqpt(p, addpt(level.glenda, Pt(0, 1))))
		return Down;
	if(eqpt(p, addpt(level.glenda, Pt(-1, 0))))
		return Left;
	if(eqpt(p, addpt(level.glenda, Pt(1, 0))))
		return Right;
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
	Event e;

	if(argc == 2) 
		levelfile = argv[1];
	else
		levelfile = LEasy;
		
	if(! loadlevels(levelfile)) {
		fprint(2, "usage: %s [levelfile]\n", argv[0]);
		exits("usage");
	}

	if(initdraw(nil, nil, "sokoban") < 0)
		sysfatal("initdraw failed: %r");
	einit(Emouse|Ekeyboard);

	allocimages();
	glenda = gright;
	eresized(0);

	for(;;) {
		switch(event(&e)) {
		case Emouse:
			m = e.mouse;
			if(m.buttons&1) {
				move(mousemove(m));
				drawscreen();
			}
			if(m.buttons&2) {
				int l;
				/* levels start from 1 */
				lmenu.lasthit = level.index;
				l=emenuhit(2, &m, &lmenu);
				if(l>=0){
					level = levels[l];
					drawlevel();
					drawscreen();
				}
			}
			if(m.buttons&4)
				switch(emenuhit(3, &m, &menu)) {
				case 0:
					level = levels[level.index];
					drawlevel();
					drawscreen();
					break;
				case 1:
					loadlevels(LEasy);
					drawlevel();
					drawscreen();
					break;
				case 2:
					loadlevels(LHard);
					drawlevel();
					drawscreen();
					break;
				case 3:
					exits(nil);
				}
			break;

		case Ekeyboard:
			if(level.done)
				break;

			switch(e.kbdc) {
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
				move(key2move(e.kbdc));
				drawscreen();
				break;
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
