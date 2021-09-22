#include	"type.h"

/*
 * vertical speed indicator
 */

typedef	struct	Vsi	Vsi;
struct	Vsi
{
	Rectangle	I;
	int		ny;
	Image*		i;
	Image*		t;
};
static	Vsi	vsi;

void
vsiinit(void)
{

	vsi.I.min.x = plane.origx + 2*plane.side;
	vsi.I.min.y = plane.origy + 1*plane.side;
	vsi.I.max.x = vsi.I.min.x + plane.side;
	vsi.I.max.y = vsi.I.min.y + plane.side;

	d_bfree(vsi.i);
	d_bfree(vsi.t);
	vsi.i = d_balloc(vsi.I, 0);
	vsi.t = d_balloc(vsi.I, 0);

	d_circle(vsi.i, addpt(vsi.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	d_circle(vsi.i, addpt(vsi.I.min, Pt(plane.side24, plane.side24)),
		plane.side14, LEVEL);
	vsi.ny = NONE;
}

void
vsiupdat(void)
{
	int t;

	t = muldiv(-plane.dz, plane.side14, 1000) + plane.side24;
	if(t > plane.side)
		t = plane.side;
	if(t < 0)
		t = 0;
	if(vsi.ny != t) {
		vsi.ny = t;
		d_copy(vsi.t, vsi.I, vsi.i);
		cross(vsi.t, NONE, vsi.ny, vsi.I.min.x, vsi.I.min.y);
		d_copy(D, vsi.I, vsi.t);
	}
}
