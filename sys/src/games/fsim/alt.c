#include	"type.h"

typedef	struct	Alt	Alt;
struct	Alt
{
	Rectangle	I;
	int		na;
	int		a;
};
static	Alt	alt;
static	long	alth1[] =
{
	450, 500,
	400, 700,
	500, 800,
	600, 700,
	550, 500,
	450, 500,
	NONE
};
static	long	alth2[] =
{
	450, 500,
	450, 850,
	500, 950,
	550, 850,
	550, 500,
	450, 500,
	NONE
};
static	long	alth3[] =
{
	500, 950,
	500, 999,
	NONE
};

void
altinit(void)
{
	int i;

	alt.I.min.x = plane.origx + 2*plane.side;
	alt.I.min.y = plane.origy + 0*plane.side;
	alt.I.max.x = alt.I.min.x + plane.side;
	alt.I.max.y = alt.I.min.y + plane.side;
	rectf(D, alt.I, F_CLR);
	alt.na = NONE;
	normal(alth1);
	normal(alth2);
	normal(alth3);
	for(i=0; i<10; i++)
		draw(i*36, alth3, &alt.I);
	circle(D, add(alt.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
}

void
altupdat(void)
{
	int t;

	t = (plane.z+5) / 10;
	if(t != alt.na) {
		doalt(plane.z);
		if(alt.na != NONE)
			doalt(alt.a);
		alt.na = t;
		alt.a = plane.z;
	}
}

void
doalt(int a)
{
	int t;

	t = muldiv(a, 360, 1000);
	draw(t/10, alth1, &alt.I);
	draw(t, alth2, &alt.I);
}
