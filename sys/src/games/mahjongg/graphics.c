#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "mahjongg.h"

int
freeup(int d, Point p)
{
	/* are we blocked from above? */
	if(d == Depth -1 || (level.board[d+1][p.x][p.y].which == 0 &&
			level.board[d+1][p.x+1][p.y].which == 0 &&
			level.board[d+1][p.x][p.y+1].which == 0 &&
			level.board[d+1][p.x+1][p.y+1].which == 0))
		return 1;
	
	return 0;
}

int
freeleft(int d, Point p)
{

	/* blocked from the left? */
	if(p.x == 0 || (level.board[d][p.x-1][p.y].which == 0 &&
		level.board[d][p.x-1][p.y+1].which == 0)) 
		return 1;

	return 0;
}

int
freeright(int d, Point p)
{		
	if(p.x == Lx-2 || (level.board[d][p.x+2][p.y].which == 0 &&
		level.board[d][p.x+2][p.y+1].which == 0))
		return 1;

	return 0;
}

int
isfree(int d, Point p)
{
	return (freeleft(d, p) || freeright(d, p)) && freeup(d, p);
}

void
clearbrick(int d, Point p)
{
	level.board[d][p.x][p.y].which = 0;
	level.board[d][p.x+1][p.y].which = 0;
	level.board[d][p.x][p.y+1].which = 0;
	level.board[d][p.x+1][p.y+1].which = 0;
}

void
resize(Point p)
{
	/* resize to the size of the current level */

	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", p.x, p.y);
		close(fd);
	}

}

void
drawbrick(int d, int x, int y)
{
	Point p;
	Rectangle r;

	p = Pt(x*(Facex/2)-(d*TileDxy), y*(Facey/2)-(d*TileDxy));
	r = Rpt(p, addpt(p, Pt(Facex, Facey)));
	r = rectaddpt(r, Pt(Depth*TileDxy, Depth*TileDxy));
	draw(img, r, tileset, nil, level.board[d][x][y].start);

	if(level.board[d][x][y].clicked)
		draw(img, r, selected, nil, ZP);

	if(level.l.d == d && eqpt(level.l.p, Pt(x, y)))
		border(img, r, 2, litbrdr, level.board[d][x][y].start);

	/* looks better without borders, uncomment to check it out with'em */
//	r = Rpt(r.min, addpt(r.min, Pt(Tilex, Tiley)));
//	draw(img, r, brdr, nil, ZP);
}


void
drawlevel(void)
{
	int d, x, y;

	draw(img, img->r, background, nil, ZP);

	for(d = 0; d < Depth; d++)
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++)
				if(level.board[d][x][y].which == 1) 
					drawbrick(d, x, y);

	draw(screen, screen->r, img, nil, ZP);
	flushimage(display, 1);
}

Brick *
bmatch(int d, Point p)
{
	int x, y;
	int ld = d;

	do {
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++)
				if(level.board[ld][x][y].which == 1 && isfree(ld, Pt(x, y)) && !eqpt(Pt(x, y), p) && 
					level.board[d][p.x][p.y].type == level.board[ld][x][y].type)
			
					return &level.board[ld][x][y];

	} while(--ld >= 0);

	return nil;
}

int
canmove(void)
{
	int d, x, y;

	for(d = Depth - 1; d >= 0; d--) 
		for(y = 0; y < Ly; y++) 
			for(x = 0; x < Lx; x++) 
				if(level.board[d][x][y].which == 1 && isfree(d, Pt(x, y)))
					if(bmatch(d, Pt(x, y)) != nil)
						return 1;
	return 0;
}

void
hint(void)
{
	Brick *b = nil;
	int d = 0, x = 0, y = 0;

	if(level.c.d != -1) {
		if((b = bmatch(level.c.d, level.c.p)) != nil) {
			d = level.c.d;
			x = level.c.p.x;
			y = level.c.p.y;
		}
	} else {
		for(d = Depth - 1; d >= 0; d--) 
			for(y = 0; y < Ly; y++) 
				for(x = 0; x < Lx; x++) 
					if(level.board[d][x][y].which == 1 && isfree(d, Pt(x, y)))
						if((b = bmatch(d, Pt(x, y))) != nil)
							goto Matched;
	}

Matched:
	if(b != nil) {
		level.board[d][x][y].clicked = 1;
		b->clicked = 1;
		drawlevel();
		sleep(500);
		if(level.c.d == -1)
			level.board[d][x][y].clicked = 0;
		b->clicked = 0;
		drawlevel();
		sleep(500);
		level.board[d][x][y].clicked = 1;
		b->clicked = 1;
		drawlevel();
		sleep(500);
		if(level.c.d == -1)
			level.board[d][x][y].clicked = 0;
		b->clicked = 0;
		drawlevel();
	}

}

void
done(void)
{
	level.done = 1;
	draw(screen, screen->r, selected, gameover, ZP);
	draw(screen, screen->r, selected, gameover, ZP);
	flushimage(display, 1);
}


void
clicked(Point coord)
{
	Point p;
	int d;

	/* ugly on purpose */

	for(d = Depth - 1; d >= 0; d--) {
		p = Pt((coord.x + TileDxy*d)/(Facex/2), (coord.y + TileDxy*d)/(Facey/2));
		switch(level.board[d][p.x][p.y].which) {
		case 0:
			break;
		case 1:
			goto Found;
		case 2:
			p = Pt(p.x-1, p.y);
			goto Found;
		case 3:
			p = Pt(p.x-1, p.y-1);
			goto Found;
		case 4:
			p = Pt(p.x, p.y-1);
			goto Found;
		}
	}

	return;

Found:
	if(freeup(d, p) && (freeleft(d, p) || freeright(d, p))) {
		if(level.c.d == -1) {
			level.c.d = d;
			level.c.p = p;
			level.board[d][p.x][p.y].clicked = 1;
		} else if(!eqpt(p, level.c.p) && 
			(level.board[d][p.x][p.y].type == level.board[level.c.d][level.c.p.x][level.c.p.y].type)) {

			clearbrick(d, p);
			clearbrick(level.c.d, level.c.p);

			level.c.d = -1;
			level.c.p = Pt(0, 0);

			level.remaining -= 2;
		} else {
			level.board[d][p.x][p.y].clicked = 0;
			level.board[level.c.d][level.c.p.x][level.c.p.y].clicked = 0;
			level.c.d = -1;
			level.c.p = Pt(0, 0);
		} 
		drawlevel();
		if(!canmove())
			done();
	}
}

void
light(Point coord)
{
	Point p;
	int d;

	/* ugly on purpose */

	for(d = Depth - 1; d >= 0; d--) {
		p = Pt((coord.x + TileDxy*d)/(Facex/2), (coord.y + TileDxy*d)/(Facey/2));
		switch(level.board[d][p.x][p.y].which) {
		case 0:
			break;
		case 1:
			goto Found;
		case 2:
			p = Pt(p.x-1, p.y);
			goto Found;
		case 3:
			p = Pt(p.x-1, p.y-1);
			goto Found;
		case 4:
			p = Pt(p.x, p.y-1);
			goto Found;
		}
	}

	return;

Found:
	if(level.l.d == d && eqpt(level.l.p, p))
		return;

	if(freeup(d, p) && (freeleft(d, p) || freeright(d, p))) {
		Point tmpp;
		int tmpd;

		tmpd = level.l.d;
		tmpp = level.l.p;

		level.l.d = d;
		level.l.p = p;
		drawbrick(d, p.x, p.y);

		/* clean up the previously lit brick */
		if(tmpd != -1 && level.board[tmpd][tmpp.x][tmpp.y].which == 1) 
			drawbrick(tmpd, tmpp.x, tmpp.y);
			
		draw(screen, screen->r, img, nil, ZP);
		flushimage(display, 1);
	} else if(level.l.d != -1) {
		d = level.l.d;
		p = level.l.p;
		level.l.d = -1;
		level.l.p = Pt(0, 0);

		if(level.board[d][p.x][p.y].which == 1) {
			drawbrick(d, p.x, p.y);
			draw(screen, screen->r, img, nil, ZP);
			flushimage(display, 1);
		}
	}
}

/* below only for testing */

Point
pmatch(int d, Point p)
{
	int x, y;
	int ld = d;

	do {
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++)
				if(level.board[ld][x][y].which == 1 && isfree(ld, Pt(x, y)) && !eqpt(Pt(x, y), p) && 
					level.board[d][p.x][p.y].type == level.board[ld][x][y].type)
			
					return Pt(x, y);

	} while(--ld >= 0);

	return Pt(-1, -1);
}

int
dmatch(int d, Point p)
{
	int x, y;
	int ld = d;

	do {
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++)
				if(level.board[ld][x][y].which == 1 && isfree(ld, Pt(x, y)) && !eqpt(Pt(x, y), p) && 
					level.board[d][p.x][p.y].type == level.board[ld][x][y].type)
			
					return ld;

	} while(--ld >= 0);

	return -1;
}

void
clearlevel(void)
{
	int d, x, y;

	for(d = Depth - 1; d >= 0; d--) 
		for(y = 0; y < Ly; y++) 
			for(x = 0; x < Lx; x++) 
				if(level.board[d][x][y].which == 1 && isfree(d, Pt(x, y)))
					if(bmatch(d, Pt(x, y)) != nil) {
						clearbrick(dmatch(d, Pt(x, y)), pmatch(d, Pt(x, y)));
						clearbrick(d, Pt(x, y));
						level.remaining -= 2;
						drawlevel();
						clearlevel();
					}
}
