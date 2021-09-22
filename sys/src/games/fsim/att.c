#include	"type.h"

/*
 * attitude indicator
 */

typedef	struct	Att	Att;
struct	Att
{
	Rectangle	I;
	int		na;
	int		nb;
	long		ah1[50];
	long		ah3[50];
	Image*		i;
	Image*		t;
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

static void	horiz(int);

void
attinit(void)
{

	att.I.min.x = plane.origx + 1*plane.side;
	att.I.min.y = plane.origy + 0*plane.side;
	att.I.max.x = att.I.min.x + plane.side;
	att.I.max.y = att.I.min.y + plane.side;

	d_bfree(att.i);
	d_bfree(att.t);
	att.i = d_balloc(att.I, 0);
	att.t = d_balloc(att.I, 0);

	d_circle(att.i, addpt(att.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	att.na = NONE;
	att.nb = NONE;
	normal(att.ah1, atth1);
	normal(att.ah3, atth3);

	dodraw(att.i, 0, att.ah3, &att.I);
	dodraw(att.i, 10, att.ah3, &att.I);
	dodraw(att.i, -10, att.ah3, &att.I);
	dodraw(att.i, 20, att.ah3, &att.I);
	dodraw(att.i, -20, att.ah3, &att.I);
	dodraw(att.i, 30, att.ah3, &att.I);
	dodraw(att.i, -30, att.ah3, &att.I);
	dodraw(att.i, 60, att.ah3, &att.I);
	dodraw(att.i, -60, att.ah3, &att.I);
}

void
attupdat(void)
{
	int t1, t2;

	if(plane.nbut && !plane.obut) {
		t1 = plane.butx - att.I.min.x;
		t2 = att.I.min.y + plane.side - plane.buty;
		if(t2 >= 0 && t2 < plane.side24) {
			if(t1 >= 0 && t1 < plane.side) {
				/* set plane horizontal */
				plane.rb = DEG(0);
				plane.ab = DEG(0);
			}
		} else
		if(t2 >= plane.side24 && t2 < plane.side) {
			if(t1 >= 0 && t1 < plane.side24) {
				/* set plane standard rate left */
				plane.rb = DEG(0);
				plane.ab = DEG(360-12);
			} else
			if(t1 >= plane.side24 && t1 < plane.side) {
				/* set plane standard rate right */
				plane.rb = DEG(0);
				plane.ab = DEG(12);
			}
		}
	}

	t1 = deg(plane.ap);
	t2 = deg(plane.ab);
	if(t1 == att.na && t2 == att.nb)
		return;
	att.na = t1;
	att.nb = t2;

	d_copy(att.t, att.I, att.i);
	dodraw(att.t, att.nb, att.ah1, &att.I);
	horiz(att.na);
	d_copy(D, att.I, att.t);
}

static void
horiz(int n)
{
	int y;

	if(n > 180)
		n -= 360;
	y = muldiv(n, plane.side, 177);
	y = att.I.min.y + plane.side24 - y;
	d_segment(att.t, Pt(att.I.min.x, y),
		Pt(att.I.max.x, y), LEVEL);
}
