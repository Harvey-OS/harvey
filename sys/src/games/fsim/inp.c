#include	"type.h"

typedef	struct	Inp	Inp;
struct	Inp
{
	Rectangle	I;
	int		nx;
	int		ny;
};
static	Inp	inp;

void
inpinit(void)
{

	inp.I.min.x = plane.origx + 1*plane.side;
	inp.I.min.y = plane.origy + 2*plane.side;
	inp.I.max.x = inp.I.min.x + plane.side;
	inp.I.max.y = inp.I.min.y + plane.side;
	rectf(D, inp.I, F_CLR);
	circle(D, add(inp.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	circle(D, add(inp.I.min, Pt(plane.side24, plane.side24)),
		plane.side14, LEVEL, F_STORE);
	segment(D, Pt(inp.I.min.x+plane.side24, inp.I.min.y),
		Pt(inp.I.min.x+plane.side24, inp.I.min.y+plane.side),
		LEVEL, F_STORE);
	segment(D, Pt(inp.I.min.x, inp.I.min.y+plane.side24),
		Pt(inp.I.min.x+plane.side, inp.I.min.y+plane.side24),
		LEVEL, F_STORE);
	inp.nx = plane.side24;
	inp.ny = plane.side24;
	spot(inp.nx, inp.ny);
}

void
inpupdat(void)
{
	int nx, ny;

	if(plane.nbut && !plane.obut) {
		nx = plane.butx - inp.I.min.x;
		if(nx >= 0 && nx < plane.side) {
			ny = plane.buty - inp.I.min.y;
			if(ny >= 0 && ny < plane.side) {
				spot(nx, ny);
				spot(inp.nx, inp.ny);
				inp.nx = nx;
				inp.ny = ny;
				plane.rb = muldiv(nx-plane.side24,
					90, plane.side);
				plane.ap = muldiv(plane.side24-ny,
					DEG(8), plane.side);
			}
		}
	}
}

void
spot(int x, int y)
{
	x += inp.I.min.x;
	y += inp.I.min.y;
	rectf(D, Rect(x-Q, y-Q, x+Q-1, y+Q-1), F_XOR);
}
