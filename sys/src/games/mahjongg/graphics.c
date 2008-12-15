#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "mahjongg.h"


/*
 * mark tiles that partially obscure the given tile.
 * relies on Depth*Dxy <= Tilex/2
 */
void
markabove(int d, int x, int y)
{
	int dx, dy;

	for(d++; d < Depth; d++)
		for(dx = -1; dx <= 2; dx++)
			for(dy = -1; dy <= 2; dy++)
				if(x+dx < Lx && x+dx >= 0 &&
				    y+dy < Ly && y+dy >= 0)
					level.board[d][x+dx][y+dy].redraw = 1;
}

void
markbelow(int d, int x, int y)
{
	int dx, dy;

	for(d--; d >= 0; d--)
		for(dx = -2; dx <= 1; dx++)
			for(dy = -2; dy <= 1; dy++)
				if(x+dx < Lx && x+dx >= 0 &&
				    y+dy < Ly && y+dy >= 0)
					level.board[d][x+dx][y+dy].redraw = 1;
}

Rectangle
tilerect(Click c)
{
	Point p;
	Rectangle r;

	p = Pt(c.x*(Facex/2)-(c.d*TileDxy), c.y*(Facey/2)-(c.d*TileDxy));
	r = Rpt(p, addpt(p, Pt(Facex, Facey)));
	return rectaddpt(r, Pt(Depth*TileDxy, Depth*TileDxy));
}

void
clearbrick(Click c)
{
	Rectangle r;

	level.hist[--level.remaining] = c;

	level.board[c.d][c.x][c.y].which = None;
	level.board[c.d][c.x+1][c.y].which = None;
	level.board[c.d][c.x][c.y+1].which = None;
	level.board[c.d][c.x+1][c.y+1].which = None;

	r = tilerect(c);
	draw(img, r, background, nil, r.min);

	markabove(c.d, c.x, c.y);
	markbelow(c.d, c.x, c.y);
}

void
drawbrick(Click c)
{
	Rectangle r;

	r = tilerect(c);
	draw(img, r, tileset, nil, level.board[c.d][c.x][c.y].start);

	if(level.board[c.d][c.x][c.y].clicked)
		draw(img, r, selected, nil, ZP);

	if(eqcl(level.l, c))
		border(img, r, 2, litbrdr, level.board[c.d][c.x][c.y].start);

	/* looks better without borders, uncomment to check it out with'em */
//	r = Rpt(r.min, addpt(r.min, Pt(Tilex, Tiley)));
//	draw(img, r, brdr, nil, ZP);
}

void
redrawlevel(int all)
{
	Brick *b;
	int d, x, y;

	for(d = 0; d < Depth; d++)
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++) {
				b = &level.board[d][x][y];
				if(b->which == TL && (all || b->redraw)) {
					drawbrick(Cl(d,x,y));
					markabove(d,x,y);
				}
				b->redraw = 0;
			}

	draw(screen, screen->r, img, nil, ZP);
	flushimage(display, 1);
}

void
updatelevel(void)
{
	redrawlevel(0);
}

void
drawlevel(void)
{
	draw(img, img->r, background, nil, ZP);
	redrawlevel(1);
}

void
resize(Point p)
{
	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", p.x, p.y);
		close(fd);
	}
}

void
hint(void)
{
	int d = 0, x = 0, y = 0;
	Brick *b = nil;

	if(level.c.d != -1) {
		if((b = bmatch(level.c)) != nil) {
			d = level.c.d;
			x = level.c.x;
			y = level.c.y;
		}
	} else 
		for(d = Depth - 1; d >= 0; d--)
			for(y = 0; y < Ly; y++)
				for(x = 0; x < Lx; x++)
					if(level.board[d][x][y].which == TL &&
					    isfree(Cl(d,x,y)) &&
					    (b = bmatch(Cl(d,x,y))) != nil)
						goto Matched;
Matched:
	if (b == nil)
		return;
	level.board[d][x][y].clicked = 1;
	b->clicked = 1;
	b->redraw = 1;
	updatelevel();
	sleep(500);
	if(level.c.d == -1)
		level.board[d][x][y].clicked = 0;
	b->clicked = 0;
	b->redraw = 1;
	updatelevel();
	sleep(500);
	level.board[d][x][y].clicked = 1;
	b->clicked = 1;
	b->redraw = 1;
	updatelevel();
	sleep(500);
	if(level.c.d == -1)
		level.board[d][x][y].clicked = 0;
	b->clicked = 0;
	b->redraw = 1;
	updatelevel();
}

void
done(void)
{
	level.done = 1;
	draw(screen, screen->r, selected, gameover, ZP);
	flushimage(display, 1);
}

Click
findclick(Point coord)
{
	Click c;

	for(c.d = Depth - 1; c.d >= 0; c.d--) {
		c.x = (coord.x + TileDxy*c.d)/(Facex/2);
		c.y = (coord.y + TileDxy*c.d)/(Facey/2);
		switch(level.board[c.d][c.x][c.y].which) {
		case None:
			break;
		case TL:
			return c;
		case TR:
			c.x = c.x - 1;
			return c;
		case BR:
			c.x = c.x - 1;
			c.y = c.y - 1;
			return c;
		case BL:
			c.y = c.y - 1;
			return c;
		}
	}
	return NC;
}

void
clicked(Point coord)
{
	Click c;
	Brick *b, *bc;

	c = findclick(coord);
	if (c.d == -1)
		return;

	b = &level.board[c.d][c.x][c.y];
	if(isfree(c)) {
		if(level.c.d == -1) {
			level.c = c;
			b->clicked = 1;
			b->redraw = 1;
		} else if(eqcl(c, level.c)) {
			level.c = NC;
			b->clicked = 0;
			b->redraw = 1;
		} else {
			bc = &level.board[level.c.d][level.c.x][level.c.y];
			if(b->type == bc->type) {
				clearbrick(c);
				bc->clicked = 0;
				clearbrick(level.c);
				level.c = NC;
			} else {
				bc->clicked = 0;
				bc->redraw = 1;
				b->clicked = 1;
				b->redraw = 1;
				level.c = c;
			}
		}
		updatelevel();
		if(!canmove())
			done();
	}
}

void
undo(void)
{
	int i, j, d, x, y;

	if(level.remaining >= Tiles)
		return;

	for(i=1; i<=2; i++) {
		j = level.remaining++;
		d = level.hist[j].d;
		x = level.hist[j].x;
		y = level.hist[j].y;
		level.board[d][x][y].which = TL;
		level.board[d][x+1][y].which = TR;
		level.board[d][x+1][y+1].which = BR;
		level.board[d][x][y+1].which = BL;
		level.board[d][x][y].redraw = 1;
	}
	updatelevel();
}

void
deselect(void)
{
	Brick *b;

	if(level.c.d == -1)
		return;
	b = &level.board[level.c.d][level.c.x][level.c.y];
	level.c = NC;
	b->clicked = 0;
	b->redraw = 1;
	updatelevel();
}

void
light(Point coord)
{
	Click c = findclick(coord);
	if (c.d == -1)
		return;

	if(eqcl(level.l, c))
		return;

	if (level.l.d != -1) {
		level.board[level.l.d][level.l.x][level.l.y].redraw = 1;
		level.l = NC;
	}

	if(isfree(c)) {
		level.l = c;
		level.board[c.d][c.x][c.y].redraw = 1;
	}

	updatelevel();
}

void
clearlevel(void)
{
	Click c, cm;

	for(c.d = Depth - 1; c.d >= 0; c.d--)
		for(c.y = 0; c.y < Ly; c.y++)
			for(c.x = 0; c.x < Lx; c.x++)
				if(level.board[c.d][c.x][c.y].which == TL &&
				    isfree(c)) {
					cm = cmatch(c, c.d);
					if(cm.d != -1) {
						clearbrick(cm);
						clearbrick(c);
						updatelevel();
						clearlevel();
					}
				}
}
