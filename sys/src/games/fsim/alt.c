#include	"type.h"

typedef	struct	Alt	Alt;
struct	Alt
{
	Rectangle	I;
	int		na;
	long		n1[50];
	long		n2[50];
	long		n3[50];
	Image*		i;
	Image*		t;
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

static	void	doalt(int);

void
altinit(void)
{
	int i;

	alt.I.min.x = plane.origx + 2*plane.side;
	alt.I.min.y = plane.origy + 0*plane.side;
	alt.I.max.x = alt.I.min.x + plane.side;
	alt.I.max.y = alt.I.min.y + plane.side;

	d_bfree(alt.i);
	d_bfree(alt.t);
	alt.i = d_balloc(alt.I, 0);
	alt.t = d_balloc(alt.I, 0);

	alt.na = NONE;
	normal(alt.n1, alth1);
	normal(alt.n2, alth2);
	normal(alt.n3, alth3);
	for(i=0; i<10; i++)
		dodraw(alt.i, i*36, alt.n3, &alt.I);
	d_circle(alt.i, addpt(alt.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
}

void
altupdat(void)
{
	int t;

	t = (plane.z + 5) / 10;
	if(t == alt.na)
		return;

	d_copy(alt.t, alt.I, alt.i);
	t = muldiv(t, 360, 100);
	dodraw(alt.t, t/10, alt.n1, &alt.I);
	dodraw(alt.t, t, alt.n2, &alt.I);
	d_copy(D, alt.I, alt.t);
	alt.na = t;
}
