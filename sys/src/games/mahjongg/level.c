#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>

#include "mahjongg.h"

void
consumeline(Biobuf *b)
{
	while(Bgetc(b) != '\n')
		;
}

/* parse a level file */
int
parse(char *layout)
{
	int x = 0, y = 0, depth = 0;
	char c;
	Biobuf *b;

	b = Bopen(layout, OREAD);
	if(b == nil) {
		fprint(2, "could not open file %s: %r\n", layout);
		return 0;
	}

	level.remaining = 0;

	while((c = Bgetc(b)) > 0) {
		switch(c)  {
		case '\n':
			x = 0;
			y = (y+1) % Ly;
			if(!y)
				depth++;
			break;
		case '.':
			orig.board[depth][x][y].which = 0;
			x++;
			break;
		case '1':
			orig.remaining++;
		case '2':
		case '3':
		case '4':
			orig.board[depth][x++][y].which = c-48;
			break;
		default:
			consumeline(b);
			break;
		}
	}
	Bterm(b);

	return 1;
}

int
indextype(int type)
{
	int t;

	if(type < 108)
		t = (type/36)*Facey * 9 + ((type%36)/4)*Facex;
	else if(type < 112)
		t = Seasons;
	else if(type < 128)
		t = 3*Facey + (((type+12)%36)/4)*Facex;
	else if(type < 132)
		t = Flowers;
	else
		t = 4*Facey + (((type+28)%36)/4)*Facex;

	return t;

}

Point
indexpt(int type)
{
	Point p;

	/*
	 * the first 108 bricks are 4 of each, 36 per line:
	 * 	x = (index%36)/4
	 *	y = (index)/36
	 * then multiply by the size of a single tile.
	 * the next 4 are the seasons, so x = index%4...
	 * and so on...
	 */

	if(type < 108)
		p = Pt(((type%36)/4)*Facex, (type/36)*Facey);
	else if(type < 112)
		p = Pt((type%4)*Facex, 3*Facey);
	else if(type < 128)
		p = Pt((((type+12)%36)/4)*Facex, 3*Facey);
	else if(type < 132)
		p = Pt(((type+4)%4)*Facex, 4*Facey);
	else
		p = Pt((((type+28)%36)/4)*Facex, 4*Facey);
	return p;
}

/* use the seed to generate a replayable game */
void
generate(uint seed)
{
	int x, y, d, n;
	int order[144];
	Point p;

	srand(seed);

	for (x = 0; x < Tiles; x++)
		order[x] = x;

	for(x = 0; x < Tiles; x++) {
		n = order[x];
		y = nrand(Tiles);
		order[x] = order[y];
		order[y] = n;
	}

	n = 0;
	for(d = 0; d < Depth; d++)
		for(y = 0; y < Ly; y++)
			for(x = 0; x < Lx; x++)
				if(orig.board[d][x][y].which == 1) {

					orig.board[d][x][y].type = indextype(order[n]);
					p = indexpt(order[n++]);
					orig.board[d][x][y].start = p;
					orig.board[d][x+1][y].start = p;
					orig.board[d][x][y+1].start = p;
					orig.board[d][x+1][y+1].start = p;
				}

	if(n != orig.remaining)
		fprint(2, "level improperly generated: %d elements, "
			"should have %d\n", n, orig.remaining);

	orig.c = NC;
	orig.l = NC;
	orig.done = 0;
	level = orig;
}
