#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>

#include "sokoban.h"

void
consumeline(Biobuf *b)
{
	while(Bgetc(b) != '\n')
		;
}

/* parse a level file */
int
loadlevels(char *path)
{
	Biobuf *b;
	int x = 0, y = 0, lnum = 0;
	char c;
		
	if(path == nil)
		return 0;

	b = Bopen(path, OREAD);
	if(b == nil) {
		fprint(2, "could not open file %s: %r\n", path);
		return 0;
	}

	memset(levels, 0, Maxlevels*sizeof(Level));
	
	while((c = Bgetc(b)) > 0) {
		switch(c)  {
		case ';':
			consumeline(b); 	/* no ';'-comments in the middle of a level */
			break;
		case '\n':
			levels[lnum].index = lnum;
			levels[lnum].done = 0;
			x = 0;
			levels[lnum].max.y = ++y;

			c = Bgetc(b);
			if(c == '\n' || c == Beof) {
				/* end of level */
				if(++lnum == Maxlevels)
					goto Done;

				x = 0;
				y = 0;
			} else
				Bungetc(b);
			break;
		case '#':
			levels[lnum].board[x][y] = Wall;
			x++;
			break;
		case ' ':
			levels[lnum].board[x][y] = Empty;
			x++;
			break;
		case '$':
			levels[lnum].board[x][y] = Cargo;
			x++;
			break;
		case '*':
			levels[lnum].board[x][y] = GoalCargo;
			x++;
			break;
		case '.':
			levels[lnum].board[x][y] = Goal;
			x++;
			break;
		case '@':
			levels[lnum].board[x][y] = Empty;
			levels[lnum].glenda = Pt(x, y);
			x++;
			break;
		case '+':
			levels[lnum].board[x][y] = Goal;
			levels[lnum].glenda = Pt(x, y);
			x++;
			break;
		default:
			fprint(2, "impossible character for level %d: %c\n", lnum+1, c);
			return 0;
		}
		if(x > levels[lnum].max.x)
			levels[lnum].max.x = x;
		levels[lnum].max.y = y;
	}
Done:
	Bterm(b);

	level = levels[0];
	numlevels = lnum;

	return 1;
}
