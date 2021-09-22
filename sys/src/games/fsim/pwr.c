#include	"type.h"

typedef	struct	Pwr	Pwr;
struct	Pwr
{
	Rectangle	I;
	int		ny;
	Image*		i;
	Image*		t;
};
static	Pwr	pwr;

void
pwrinit(void)
{

	pwr.I.min.x = plane.origx + 2*plane.side;
	pwr.I.min.y = plane.origy + 2*plane.side;
	pwr.I.max.x = pwr.I.min.x + plane.side;
	pwr.I.max.y = pwr.I.min.y + plane.side;

	d_bfree(pwr.i);
	d_bfree(pwr.t);
	pwr.i = d_balloc(pwr.I, 0);
	pwr.t = d_balloc(pwr.I, 0);

	d_circle(pwr.i, addpt(pwr.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
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
	if(pwr.ny == t)
		return;

	pwr.ny = t;
	d_copy(pwr.t, pwr.I, pwr.i);
	d_set(pwr.t, Rect(pwr.I.min.x + plane.side24 - Q,
		pwr.I.min.y + pwr.ny,
		pwr.I.min.x + plane.side24 + Q - 1,
		pwr.I.min.y + plane.side));
	d_copy(D, pwr.I, pwr.t);
}
