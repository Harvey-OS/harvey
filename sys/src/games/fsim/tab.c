#include	"type.h"

/*
 * turn and bank
 */

typedef	struct	Tab	Tab;
struct	Tab
{
	Rectangle	I;
	int		na;
	long		n1[50];
	long		n2[50];
	Image*		i;
	Image*		t;
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

	d_bfree(tab.i);
	d_bfree(tab.t);
	tab.i = d_balloc(tab.I, 0);
	tab.t = d_balloc(tab.I, 0);

	d_circle(tab.i, addpt(tab.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	tab.na = NONE;
	normal(tab.n1, tabh1);
	normal(tab.n2, tabh2);
	dodraw(tab.i, 90, tab.n2, &tab.I);
	dodraw(tab.i, -90, tab.n2, &tab.I);
	dodraw(tab.i, 110, tab.n2, &tab.I);
	dodraw(tab.i, -110, tab.n2, &tab.I);
}

void
tabupdat(void)
{
	int t;

	t = muldiv(isin(plane.ab), 52, plane.dx);
	if(t != tab.na) {
		tab.na = t;
		d_copy(tab.t, tab.I, tab.i);
		dodraw(tab.t, tab.na, tab.n1, &tab.I);
		d_copy(D, tab.I, tab.t);
	}
}
