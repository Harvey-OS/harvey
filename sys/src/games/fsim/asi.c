#include	"type.h"

typedef	struct	Asi	Asi;
struct	Asi
{
	Rectangle	I;
	char		s[10];
	Point		tim;
};
static	Asi	asi;

void
asiinit(void)
{

	asi.I.min.x = plane.origx + 0*plane.side;
	asi.I.min.y = plane.origy + 0*plane.side;
	asi.I.max.x = asi.I.min.x + plane.side;
	asi.I.max.y = asi.I.min.y + plane.side;
	rectf(D, asi.I, F_CLR);
	circle(D, add(asi.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	asi.tim.x = asi.I.min.x + plane.side24 - (3*W)/2;
	asi.tim.y = asi.I.min.y + plane.side24 - H/2;
	strcpy(asi.s, "");
}

void
asiupdat(void)
{
	char s[10];
	int t;

	strcpy(s, "XXX");
	t = plane.dx / KNOT;
	dconv(t, s, 3);
	if(strcmp(s, asi.s)) {
		string(D, asi.tim, font, s, F_XOR);
		string(D, asi.tim, font, asi.s, F_XOR);
		strcpy(asi.s, s);
	}
}
