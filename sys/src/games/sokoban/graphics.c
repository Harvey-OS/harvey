#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "sokoban.h"

void
drawscreen(void)
{
	draw(screen, screen->r, img, nil, ZP);
	flushimage(display, 1);
}

void
drawglenda(void)
{
	Rectangle r;
	Point p;

	p = level.glenda;
	p.x *= BoardX;
	p.y *= BoardY;
	/* leave some room from the edge of the window */
	p = addpt(p, Pt(1, 1));

	r = Rpt(p, Pt(p.x + BoardX, p.y+BoardY));
	draw(img, r, glenda, nil, ZP);
}

void
drawwin(void)
{
	Rectangle r;
	Point p;

	p = level.glenda;
	p.x *= BoardX;
	p.y *= BoardY;
	p = addpt(p, Pt(6, 6));
	p = addpt(p, Pt(1, 1));

	r = Rpt(p, Pt(p.x + BoardX, p.y+BoardY));
	draw(img, r, text, win, ZP);
}

void
drawboard(Point p)
{
	Rectangle r;
	uint square = level.board[p.x][p.y];

	p.x *= BoardX;
	p.y *= BoardY;

	/* leave some room from the edge of the window */
	p = addpt(p, Pt(1, 1));

	r = Rpt(p, Pt(p.x + BoardX, p.y+BoardY));

	switch(square) {
	case Background:
		draw(img, r, bg, nil, ZP);
		break;
	case Empty:
		draw(img, r, empty, nil, ZP);
		break;
	case Wall:
		draw(img, r, wall, nil, ZP);
		break;
	case Cargo:
		draw(img, r, cargo, nil, ZP);
		break;
	case Goal:
		draw(img, r, goal, nil, ZP);
		break;
	case GoalCargo:
		draw(img, r, goalcargo, nil, ZP);
		break;
	}
}

void
resize(Point p)
{
	/* resize to the size of the current level */

	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", p.x*BoardX+10, p.y*BoardY+10);
		close(fd);
	}

}

Point
boardsize(Point p)
{
	return Pt(p.x*BoardX+2, p.y*BoardY+2);
}

void
drawlevel(void)
{
	int x, y;

	resize(level.max);

	if(img)
		freeimage(img);
	img = eallocimage(Rpt(Pt(0, 0), boardsize(level.max)), 0, 0);	

	draw(img, insetrect(img->r, 1), empty, nil, ZP);

	for(x = 0; x < MazeX; x++) {
		for(y = 0; y < MazeY; y++) {
			drawboard(Pt(x, y));
		}
	}

	drawglenda();
}
