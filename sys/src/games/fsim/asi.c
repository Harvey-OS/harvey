#include	"type.h"

/*
 * air speed indicator
 */
typedef	struct	Asi	Asi;
struct	Asi
{
	Rectangle	I;
	char		s[10];
	char		m[10];
	Point		tim;
	Point		mag;
	Image*		i;
	Image*		t;
};
static	Asi	asi;

void
asiinit(void)
{

	asi.I.min.x = plane.origx + 0*plane.side;
	asi.I.min.y = plane.origy + 0*plane.side;
	asi.I.max.x = asi.I.min.x + plane.side;
	asi.I.max.y = asi.I.min.y + plane.side;

	d_bfree(asi.i);
	d_bfree(asi.t);
	asi.i = d_balloc(asi.I, 0);
	asi.t = d_balloc(asi.I, 0);

	d_circle(asi.i, addpt(asi.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);

	asi.tim.x = asi.I.min.x + plane.side24 - (3*W)/2;
	asi.tim.y = asi.I.min.y + plane.side24 - H/2;
	strcpy(asi.s, "");

	asi.mag.x = asi.I.min.x + plane.side24 - (3*W)/2;
	asi.mag.y = asi.I.min.y + plane.side24 - (5*H)/2;
	strcpy(asi.m, "");
}

void
asiupdat(void)
{
	char s[10], m[10];
	int t;

	strcpy(s, "XXX");
	t = plane.dx / KNOT(1);
	dconv(t, s, 3);

	strcpy(m, "XXX");
	t = plane.magvar / DEG(1);
	dconv(t, m, 3);

	if(!strcmp(s, asi.s) && !strcmp(m, asi.m))
		return;

	d_copy(asi.t, asi.I, asi.i);
	strcpy(asi.s, s);
	strcpy(asi.m, m);
	d_string(asi.t, asi.tim, font, asi.s);
	d_string(asi.t, asi.mag, font, asi.m);
	d_copy(D, asi.I, asi.t);
}
