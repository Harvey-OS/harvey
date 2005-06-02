/* code from mark huckvale: http://www.phon.ucl.ac.uk/home/mark/sudoku/ */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "sudoku.h"

char *imgdir = "/sys/games/lib/sudoku/images";
char *lvldir = "/sys/games/lib/sudoku/boards";	/* level library dir */

int selected;	/* which digit do we have selected? */

Image *background;	/* DPaleyellow */
Image *backselect;	/* DPalebluegreen */
Image *blink;		/* DDarkyellow */
Image *brdr;		/* 0x55555555 */
Image *fixed;		/* DBlue */
Image *wrong;		/* DRed */
Image *dig[10];		/* digit masks */

Dir *dir;
int numlevels;
int curlevel;

char *buttons[] = 
{
	"new",
	"check",
	"solve",
	"clear",
	"save",
	"load",
	"print",
	"offline",
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

int
readlevels(char *leveldir)
{
	int fd, n;

	if((fd = open(leveldir, OREAD)) < 0)
		return -1;

	n = dirreadall(fd, &dir);
	close(fd);

	return n;	
}

char *
genlevels(int i)
{
	if(numlevels == 0)
		numlevels = readlevels(lvldir);
	
	if(numlevels > 0 && i < numlevels)
		return (dir+i)->name;

	return nil;
}

void
convert(Cell *brd, int *board)
{
	int i;
	
	for(i = 0; i < Psize; i++) {
		brd[i].digit = board[i] & Digit;
		if(brd[i].digit < 0 || brd[i].digit > 9)
			brd[i].digit = -1;
		brd[i].solve = (board[i] & Solve) >> 4;
		brd[i].locked = board[i] & MLock;
	}
	memcpy(obrd, brd, Psize * sizeof(Cell));
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
clearboard(Cell *board)
{
	int i;
	
	for(i = 0; i < Psize; i++) {
		board[i].digit = -1;
		board[i].solve = 0;
		board[i].locked = 0;
	}
}

void
solveboard(Cell *board)
{
	int i;

	for(i = 0; i < Psize; i++) {
		board[i].digit = board[i].solve;
	}
}


int
checkpossible(Cell *board, int x, int y, int num)
{
	int i, j;

	for(i = 0; i < Brdsize; i++) {
		if(board[i*Brdsize + y].digit == num && i != x)
			return 0;
		if(board[x*Brdsize + i].digit == num && i != y)	
			return 0;
	}

	for(i = x - (x%3); i < x - (x%3) + 3; i++)
		for(j = y - (y%3); j < y - (y%3) + 3; j++)
			if((i != x && j != y) && board[i*Brdsize + j].digit == num)
				return 0;

	return 1;
}

void
resize(void)
{
	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", Maxx, Maxy);
		close(fd);
	}

}

void
drawcell(int x, int y, int num, Image *col)
{
	Rectangle r = Rect(x*Square, y*Square, (x+1)*Square, (y+1)*Square);

	if(num < 0 || num > 9)
		return;

	r = insetrect(r, Border);
	r = rectaddpt(r, Pt(0, Square));
	r.max = addpt(r.max, Pt(2, 2));
	
	draw(screen, rectaddpt(r, screen->r.min), col, dig[num], ZP);
}

void
drawboard(void)
{
	int i;

	for(i = 0; i < Psize; i++) {
		drawcell(i / Brdsize, i % Brdsize, brd[i].digit, brd[i].locked ? fixed : display->black);
	}
}

void
drawchecked(Cell *brd)
{
	int i;

	for(i = 0; i < Psize; i++) {
		if(brd[i].locked)
			drawcell(i / Brdsize, i % Brdsize, brd[i].digit, fixed);
		else 
			drawcell(i / Brdsize, i % Brdsize, brd[i].digit, 
					checkpossible(brd, i / Brdsize, i % Brdsize, brd[i].digit) ? display->black : wrong);
	}
}

void
drawscreen(void)
{
	Point l1, l2;
	int i;

	draw(screen, screen->r, brdr, nil, ZP);
	draw(screen, insetrect(screen->r, Border), background, nil, ZP);
	for(i = 0; i < Brdsize; i++) {
		l1 = addpt(screen->r.min, Pt(i*Square, Square));
		l2 = addpt(screen->r.min, Pt(i*Square, Maxy));
		line(screen, l1, l2, Endsquare, Endsquare, (i%3) == 0 ? Thickline : Line, brdr, ZP); 
		l1 = addpt(screen->r.min, Pt(0, (i+1)*Square));
		l2 = addpt(screen->r.min, Pt(Maxx, (i+1)*Square));
		line(screen, l1, l2, Endsquare, Endsquare, (i%3) == 0 ? Thickline : Line, brdr, ZP); 
	}
	for(i = 1; i < 10; i++) {
		drawbar(i, (selected == i) ? 1 : 0);
	}
	drawboard();
	flushimage(display, 1);
}


void
drawbar(int digit, int selected)
{
	Rectangle r = Rect((digit - 1)*Square, 0, digit*Square, Square);

	if(digit < 1 || digit > 9)
		return;

	r = insetrect(r, Border);
	r.max = addpt(r.max, Pt(2, 2));
	draw(screen, rectaddpt(r, screen->r.min), selected ? backselect : background, nil, ZP);
	draw(screen, rectaddpt(r, screen->r.min), display->black, dig[digit-1], ZP);
}

void
eresized(int new)
{
	Point p;
	char path[256];
	int i;

	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");
	
	if(background == nil) 
		background = eallocimage(Rect(0, 0, 1, 1), 1, DPaleyellow);
	if(backselect == nil) 
		backselect = eallocimage(Rect(0, 0, 1, 1), 1, DPalebluegreen);
	if(blink == nil) 
		blink = eallocimage(Rect(0, 0, 1, 1), 1, DDarkyellow);
	if(brdr == nil)
		brdr = eallocimage(Rect(0, 0, 1, 1), 1, 0x55555555);
	if(fixed == nil)
		fixed = eallocimage(Rect(0, 0, 1, 1), 1, DBlue);
	if(wrong == nil)
		wrong = eallocimage(Rect(0, 0, 1, 1), 1, DRed);
	if(dig[0] == nil) {
		for(i = 0; i < 9; i++) {
			snprint(path, 256, "%s/%d.bit", imgdir, i+1);
			dig[i] = eloadfile(path);
		}
	}

	p = Pt(Dx(screen->r), Dy(screen->r));
	if(!new || !eqpt(p, Pt(Maxx - 8, Maxy - 8)))
		resize();

	drawscreen();
}

void
main(int argc, char *argv[])
{
	Mouse m;
	Event e;
	Point p;
	int last1 = 0;	/* was the button clicked last time? */

	USED(argc, argv);

	if(initdraw(nil, nil, "sudoku") < 0)
		sysfatal("initdraw failed: %r");

	einit(Emouse|Ekeyboard);


	clearboard(brd);
	eresized(0);

	srand(time(0)*getpid());
	makep();
	convert(brd, board);

	drawscreen();
	for(;;) {
		switch(event(&e)) {
		case Emouse:
			m = e.mouse;
			if(m.buttons&1) {
				if(last1 == 0) {
					last1 = 1;
					p = subpt(m.xy, screen->r.min);
					if(ptinrect(p, Rect(0, 0, Maxx, Square+Border))) {
						if(p.x/Square == selected - 1) {
							drawbar(selected, 0);
							selected = 0;
						} else {
							selected = p.x/Square + 1;
						}
					} else {
						Point lp = divpt(p, Square);
						lp.y--;

						if(brd[lp.x * Brdsize + lp.y].locked)
							break;

						if(selected) {
							brd[lp.x * Brdsize + lp.y].digit = selected - 1;
						} else {
							brd[lp.x * Brdsize + lp.y].digit = -1;
						}			
					}
					drawscreen();
				}
			} else {
				last1 = 0;
			}

			if(m.buttons&2) {
				char *str;
				int l;
				/* levels start from 1 */
				lmenu.lasthit = curlevel;
				l = emenuhit(2, &m, &lmenu);
				if(l >= 0){
					curlevel = l;
					str = smprint("%s/%s", lvldir, (dir+curlevel)->name);
					if(loadlevel(str, brd) < 0)
						clearboard(brd);
					memcpy(obrd, brd, Psize * sizeof(Cell));
					free(str);
				}
				drawscreen();
			}
			if(m.buttons&4) {
				switch(emenuhit(3, &m, &menu)) {
				case 0: 	/* new */
					makep();
					convert(brd, board);
					drawscreen();
					break;
				case 1:		/* solve */
					drawchecked(brd);
					break;
				case 2:		/* solve */
					solveboard(brd);
					drawscreen();
					break;
				case 3:		/* clear */
					memcpy(brd, obrd, Psize * sizeof(Cell));
					drawscreen();
					break;
				case 4:		/* save */
					savegame(brd);
					drawscreen();
					break;
				case 5:		/* load */
					if(loadgame(brd) < 0) {
						clearboard(brd);
					}
					memcpy(obrd, brd, Psize * sizeof(Cell));
					drawscreen();
					break;
				case 6:		/* print */
					printboard(brd);
					break;
				case 7:		/* offline */
					fprettyprintbrd(brd);
					break;
				case 8:		/* exit */
					exits(nil);
				}
			}
			break;

		case Ekeyboard:
			switch(e.kbdc) {
			case 127:
			case 'q':
			case 'Q':
				exits(nil);
			default:
				break;
			}
			break;
		}
	}
}
