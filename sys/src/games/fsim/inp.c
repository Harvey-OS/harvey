#include	"type.h"

/*
 * input indicator
 * (stick)
 */
typedef	struct	Inp	Inp;
struct	Inp
{
	Rectangle	I;
	int		nx;
	int		ny;
	Image*		i;
	Image*		t;
};
static	Inp	inp;

static	void	spot(int, int);

void
inpinit(void)
{

	inp.I.min.x = plane.origx + 1*plane.side;
	inp.I.min.y = plane.origy + 2*plane.side;
	inp.I.max.x = inp.I.min.x + plane.side;
	inp.I.max.y = inp.I.min.y + plane.side;

	d_bfree(inp.i);
	d_bfree(inp.t);
	inp.i = d_balloc(inp.I, 0);
	inp.t = d_balloc(inp.I, 0);

	d_circle(inp.i, addpt(inp.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	d_circle(inp.i, addpt(inp.I.min, Pt(plane.side24, plane.side24)),
		plane.side14, LEVEL);
	d_segment(inp.i, Pt(inp.I.min.x+plane.side24, inp.I.min.y),
		Pt(inp.I.min.x+plane.side24, inp.I.min.y+plane.side),
		LEVEL);
	d_segment(inp.i, Pt(inp.I.min.x, inp.I.min.y+plane.side24),
		Pt(inp.I.min.x+plane.side, inp.I.min.y+plane.side24),
		LEVEL);
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
				inp.nx = nx;
				inp.ny = ny;
				spot(inp.nx, inp.ny);
				plane.rb = muldiv(nx-plane.side24,
					90, plane.side);
				plane.ap = muldiv(plane.side24-ny,
					DEG(8), plane.side);
			}
		}
	}
}

static void
spot(int x, int y)
{
	d_copy(inp.t, inp.I, inp.i);
	x += inp.I.min.x;
	y += inp.I.min.y;
	d_set(inp.t, Rect(x-Q, y-Q, x+Q-1, y+Q-1));
	d_copy(D, inp.I, inp.t);
}
