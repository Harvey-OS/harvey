#include	"type.h"

typedef	struct	Pwr	Pwr;
struct	Pwr
{
	Rectangle	I;
	int		ny;
};
static	Pwr	pwr;

void
pwrinit(void)
{

	pwr.I.min.x = plane.origx + 2*plane.side;
	pwr.I.min.y = plane.origy + 2*plane.side;
	pwr.I.max.x = pwr.I.min.x + plane.side;
	pwr.I.max.y = pwr.I.min.y + plane.side;
	rectf(D, pwr.I, F_CLR);
	circle(D, add(pwr.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	pwr.ny = NONE;
}

void
pwrupdat(void)
{
	int t;

	if(plane.nbut && !plane.obut) {
		t = plane.butx - pwr.I.min.x;
		if(t >= plane.side14 && t < plane.side34) {
			t = pwr.I.min.y + plane.side - plane.buty;
			if(t >= 0 && t < plane.side)
				plane.pwr = muldiv(t, 1000, plane.side);
		}
	}
	t = plane.side -  muldiv(plane.pwr, plane.side, 1000);
	if(pwr.ny != t) {
		rectf(D, Rect(pwr.I.min.x + plane.side24 - Q,
			pwr.I.min.y + t,
			pwr.I.min.x + plane.side24 + Q - 1,
			pwr.I.min.y + plane.side), F_XOR);
		if(pwr.ny != NONE)
		rectf(D, Rect(pwr.I.min.x + plane.side24 - Q,
			pwr.I.min.y + pwr.ny,
			pwr.I.min.x + plane.side24 + Q - 1,
			pwr.I.min.y + plane.side), F_XOR);
	}
	pwr.ny = t;
}
