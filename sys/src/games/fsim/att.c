#include	"type.h"

typedef	struct	Att	Att;
struct	Att
{
	Rectangle	I;
	int		na;
	int		nb;
};
static	Att	att;
static	long	atth1[] =
{
	0, 550,
	300, 550,
	300, 450,
	700, 450,
	700, 550,
	999, 550,
	BREAK,
	500, 450,
	500, 950,
	NONE
};
static	long	atth3[] =
{
	500, 950,
	500, 999,
	NONE
};

void
attinit(void)
{

	att.I.min.x = plane.origx + 1*plane.side;
	att.I.min.y = plane.origy + 0*plane.side;
	att.I.max.x = att.I.min.x + plane.side;
	att.I.max.y = att.I.min.y + plane.side;
	rectf(D, att.I, F_CLR);
	circle(D, add(att.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	att.na = NONE;
	att.nb = NONE;
	normal(atth1);
	normal(atth3);
	draw(0, atth3, &att.I);
	draw(10, atth3, &att.I);
	draw(-10, atth3, &att.I);
	draw(20, atth3, &att.I);
	draw(-20, atth3, &att.I);
	draw(30, atth3, &att.I);
	draw(-30, atth3, &att.I);
	draw(60, atth3, &att.I);
	draw(-60, atth3, &att.I);
}

void
attupdat(void)
{
	int t1, t2;

	t1 = deg(plane.ap);
	t2 = deg(plane.ab);
	if(t1 == att.na && t2 == att.nb)
		return;
	draw(t2, atth1, &att.I);
	horiz(t1);
	if(att.nb != NONE) {
		draw(att.nb, atth1, &att.I);
		horiz(att.na);
	}
	att.na = t1;
	att.nb = t2;
}

void
horiz(int n)
{
	int y;

	if(n > 180)
		n -= 360;
	y = muldiv(n, plane.side, 177);
	y = att.I.min.y + plane.side24 - y;
	segment(D, Pt(att.I.min.x, y),
		Pt(att.I.max.x, y), LEVEL, F_XOR);
}
