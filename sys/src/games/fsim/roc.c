#include	"type.h"

typedef	struct	Roc	Roc;
struct	Roc
{
	Rectangle	I;
	int		ny;
};
static	Roc	roc;

void
rocinit(void)
{

	roc.I.min.x = plane.origx + 2*plane.side;
	roc.I.min.y = plane.origy + 1*plane.side;
	roc.I.max.x = roc.I.min.x + plane.side;
	roc.I.max.y = roc.I.min.y + plane.side;
	rectf(D, roc.I, F_CLR);
	circle(D, add(roc.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	circle(D, add(roc.I.min, Pt(plane.side24, plane.side24)),
		plane.side14, LEVEL, F_STORE);
	roc.ny = NONE;
}

void
rocupdat(void)
{
	int t;

	t = muldiv(-plane.dz, plane.side14, 1000) + plane.side24;
	if(t > plane.side)
		t = plane.side;
	if(t < 0)
		t = 0;
	if(roc.ny != t) {
		cross(NONE, t, roc.I.min.x, roc.I.min.y);
		cross(NONE, roc.ny, roc.I.min.x, roc.I.min.y);
	}
	roc.ny = t;
}
