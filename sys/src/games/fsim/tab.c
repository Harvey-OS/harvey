#include	"type.h"

typedef	struct	Tab	Tab;
struct	Tab
{
	Rectangle	I;
	int		na;
};

static	Tab	tab;
static	long	tabh1[] =
{
	100, 500,
	900, 500,
	600, 600,
	570, 650,
	700, 650,
	525, 700,
	500, 800,
	475, 700,
	300, 650,
	430, 650,
	400, 600,
	100, 500,
	NONE
};
static	long	tabh2[] =
{
	500, 900,
	500, 999,
	NONE
};

void
tabinit(void)
{

	tab.I.min.x = plane.origx + 0*plane.side;
	tab.I.min.y = plane.origy + 1*plane.side;
	tab.I.max.x = tab.I.min.x + plane.side;
	tab.I.max.y = tab.I.min.y + plane.side;
	rectf(D, tab.I, F_CLR);
	circle(D, add(tab.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	tab.na = NONE;
	normal(tabh1);
	normal(tabh2);
	draw(90, tabh2, &tab.I);
	draw(-90, tabh2, &tab.I);
	draw(110, tabh2, &tab.I);
	draw(-110, tabh2, &tab.I);
}

void
tabupdat(void)
{
	int t;

	t = muldiv(isin(plane.ab), 52, plane.dx);
	if(t != tab.na) {
		draw(t, tabh1, &tab.I);
		if(tab.na != NONE)
		draw(tab.na, tabh1, &tab.I);
		tab.na = t;
	}
}
