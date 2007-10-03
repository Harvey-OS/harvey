#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

#include "mahjongg.h"

Click NC = { -1, 0, 0, };

Click
Cl(int d, int x, int y)
{
	return (Click){d, x, y};
}

int
eqcl(Click c1, Click c2)
{
	return c1.d == c2.d && c1.x == c2.x && c1.y == c2.y;
}

int
freeup(Click c)
{
	if(c.d == Depth -1 || (level.board[c.d+1][c.x][c.y].which == None &&
	    level.board[c.d+1][c.x+1][c.y].which == None &&
	    level.board[c.d+1][c.x][c.y+1].which == None &&
	    level.board[c.d+1][c.x+1][c.y+1].which == None))
		return 1;
	return 0;
}

int
freeleft(Click c)
{
	if(c.x == 0 || (level.board[c.d][c.x-1][c.y].which == None &&
	    level.board[c.d][c.x-1][c.y+1].which == None))
		return 1;
	return 0;
}

int
freeright(Click c)
{
	if(c.x == Lx-2 || (level.board[c.d][c.x+2][c.y].which == None &&
	    level.board[c.d][c.x+2][c.y+1].which == None))
		return 1;
	return 0;
}

int
isfree(Click c)
{
	return (freeleft(c) || freeright(c)) && freeup(c);
}

Click
cmatch(Click c, int dtop)
{
	Click lc;

	lc.d = dtop;
	do {
		for(lc.y = 0; lc.y < Ly; lc.y++)
			for(lc.x = 0; lc.x < Lx; lc.x++)
				if(level.board[lc.d][lc.x][lc.y].which == TL &&
				    isfree(lc) && !eqcl(c, lc) &&
				    level.board[c.d][c.x][c.y].type ==
				    level.board[lc.d][lc.x][lc.y].type)
					return lc;
	} while(--lc.d >= 0);
	return NC;
}

Brick *
bmatch(Click c)
{
	Click lc;

	lc = cmatch(c, Depth);
	if(lc.d == -1)
		return nil;
	else
		return &level.board[lc.d][lc.x][lc.y];
}

int
canmove(void)
{
	Click c;

	for(c.d = Depth - 1; c.d >= 0; c.d--)
		for(c.y = 0; c.y < Ly; c.y++)
			for(c.x = 0; c.x < Lx; c.x++)
				if(level.board[c.d][c.x][c.y].which == TL &&
				    isfree(c) && bmatch(c) != nil)
					return 1;
	return 0;
}
