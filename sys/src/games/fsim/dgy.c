#include	"type.h"

typedef	struct	Dgy	Dgy;
struct	Dgy
{
	Rectangle	I;
	long		n1[50];
	long		n00[50];
	long		n09[50];
	long		n18[50];
	long		n27[50];
	Image*		i;
	Image*		t;
};
static	Dgy	dgy;
static	Image*	rose[30];
static	long	dgyh1[] =	/* airplane */
{
	-25+500, -200+500,
	-25+500, 200+500,
	-75+500, 200+500,
	0+500, 275+500,
	75+500, 200+500,
	25+500, 200+500,
	25+500, -200+500,
	-25+500, -200+500,
	BREAK,
	-20+500, 999,
	500, 950,
	20+500, 999,
	NONE
};
static	long	dgyh00[] =	/* marker N */
{
	475, 800,
	475, 875,
	525, 800,
	525, 875,
	NONE
};
static	long	dgyh09[] =	/* marker E */
{
	525, 875,
	475, 875,
	475, 800,
	525, 800,
	BREAK,
	525, 837,
	475, 837,
	NONE
};
static	long	dgyh18[] =	/* marker S */
{
	525, 857,
	509, 875,
	493, 875,
	475, 857,
	493, 837,
	509, 837,
	525, 819,
	509, 800,
	493, 800,
	475, 819,
	NONE
};
static	long	dgyh27[] =	/* marker W */
{
	475, 875,
	475, 800,
	500, 837,
	525, 800,
	525, 875,
	NONE
};
static	long*	dgyhp[] =
{
	dgyh1,	dgy.n1,
	dgyh00,	dgy.n00,
	dgyh09,	dgy.n09,
	dgyh18,	dgy.n18,
	dgyh27,	dgy.n27,
};

void
dgyinit(void)
{
	int i, j;
	Fract is, ic;
	long d1, d2;
	Image *b;
	Point Po, Pn;
	Image *tmp;

	dgy.I.min.x = plane.origx + 1*plane.side;
	dgy.I.min.y = plane.origy + 1*plane.side;
	dgy.I.max.x = dgy.I.min.x + plane.side;
	dgy.I.max.y = dgy.I.min.y + plane.side;

	d_bfree(dgy.i);
	d_bfree(dgy.t);
	dgy.i = d_balloc(dgy.I, 0);
	dgy.t = d_balloc(dgy.I, 0);

	d_circle(dgy.i, addpt(dgy.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	for(i=0; i<nelem(dgyhp); i+=2)
		normal(dgyhp[i+1], dgyhp[i]);
	for(i=0; i<30; i++) {
		b = d_balloc(Rect(0,0, plane.side, plane.side), 0);
		d2 = plane.side24;
		for(j=0; j<360; j+=10) {
			is = isin(DEG(i+j));
			ic = icos(DEG(i+j));
			d1 = muldiv(plane.side24, 90, 100);
			if(j%30 == 0)
				d1 = muldiv(plane.side24, 80, 100);
			d_segment(b,
				Pt(plane.side24 + (int)fmul(d1, is),
					plane.side24 + (int)fmul(d1, ic)),
				Pt(plane.side24 + (int)fmul(d2, is),
					plane.side24 + (int)fmul(d2, ic)),
				LEVEL);
		}
		for(j=2; dgyh1[j]!=NONE; j+=2) {
			if(dgyh1[j]==BREAK)
				j += 3;
			Po.x = dgyh1[j-2] + plane.side24;
			Po.y = -dgyh1[j-1] + plane.side24;
			Pn.x = dgyh1[j+0] + plane.side24;
			Pn.y = -dgyh1[j+1] + plane.side24;
			d_segment(b, Po, Pn, LEVEL);
		}
		d_circle(b, Pt(plane.side24, plane.side24), plane.side24, LEVEL);
		tmp = d_balloc(Rect(0,0, plane.side,plane.side), 0);
		draw(tmp, tmp->r, display->black, b, b->r.min);
		rose[i] = tmp;
		freeimage(b);
	}
}

void
dgyupdat(void)
{
	int t, u, i;

	d_copy(dgy.t, dgy.I, dgy.i);
	u = deg(plane.ah);
	t = u%30;
	d_or(dgy.t, dgy.I, rose[t]);

	t = 0;
	for(i=2; i<nelem(dgyhp); i+=2) {
		dodraw(dgy.t, t-u, dgyhp[i+1], &dgy.I);
		t += 90;
	}
	d_copy(D, dgy.I, dgy.t);
}
