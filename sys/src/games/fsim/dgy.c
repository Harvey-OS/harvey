#include	"type.h"

typedef	struct	Dgy	Dgy;
struct	Dgy
{
	Rectangle	I;
	int		na[13];
};
static	Dgy	dgy;
static	Bitmap*	rose[30];
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
static	long	*dgyhp[5] =
{
	dgyh1,
	dgyh00,
	dgyh09,
	dgyh18,
	dgyh27,
};

void
dgyinit(void)
{
	int i, j;
	Fract is, ic;
	long d1, d2;
	Bitmap *b;
	Point Po, Pn;

	dgy.I.min.x = plane.origx + 1*plane.side;
	dgy.I.min.y = plane.origy + 1*plane.side;
	dgy.I.max.x = dgy.I.min.x + plane.side;
	dgy.I.max.y = dgy.I.min.y + plane.side;
	rectf(D, dgy.I, F_CLR);
	circle(D, add(dgy.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	normal(dgyh1);
	for(i=0; i<30; i++) {
		b = balloc(Rect(0,0, plane.side, plane.side), 0);
		if(!b)
		exits("1");
		rectf(b, b->r, F_CLR);
		d2 = plane.side24;
		for(j=0; j<360; j+=10) {
			is = isin(DEG(i+j));
			ic = icos(DEG(i+j));
			d1 = muldiv(plane.side24, 90, 100);
			if(j%30 == 0)
				d1 = muldiv(plane.side24, 80, 100);
			segment(b,
				Pt(plane.side24 + (int)fmul(d1, is),
					plane.side24 + (int)fmul(d1, ic)),
				Pt(plane.side24 + (int)fmul(d2, is),
					plane.side24 + (int)fmul(d2, ic)),
				LEVEL, F_STORE);
		}
		for(j=2; dgyh1[j]!=NONE; j+=2) {
			if(dgyh1[j]==BREAK)
				j += 3;
			Po.x = dgyh1[j-2] + plane.side24;
			Po.y = -dgyh1[j-1] + plane.side24;
			Pn.x = dgyh1[j+0] + plane.side24;
			Pn.y = -dgyh1[j+1] + plane.side24;
			segment(b, Po, Pn, LEVEL, F_STORE);
		}
		circle(b, Pt(plane.side24, plane.side24), plane.side24, LEVEL, F_CLR);
		rose[i] = b;
	}
	for(i=0; i<5; i++) {
		if(i)
			normal(dgyhp[i]);
		dgy.na[i] = NONE;
	}
}

void
dgyupdat(void)
{
	int t, u;
	static i = 0;

	u = deg(plane.ah);
	if(u != dgy.na[0]) {
		t = u%30;
		bitblt(D, dgy.I.min,
			rose[t], rose[t]->r, F_XOR);
		if(dgy.na[0] != NONE) {
			t = dgy.na[0]%30;
			bitblt(D, dgy.I.min,
				rose[t], rose[t]->r, F_XOR);
		}
		dgy.na[0] = u;
	}
	i = !i;
	if(u != dgy.na[i+1]) {
		t = i*90;
		draw(t-u, dgyhp[i+1], &dgy.I);
		if(dgy.na[i+1] != NONE)
			draw(t-dgy.na[i+1], dgyhp[i+1], &dgy.I);
		dgy.na[i+1] = u;
	}
	if(u != dgy.na[i+3]) {
		t = i*90 + 180;
		draw(t-u, dgyhp[i+3], &dgy.I);
		if(dgy.na[i+3] != NONE)
			draw(t-dgy.na[i+3], dgyhp[i+3], &dgy.I);
		dgy.na[i+3] = u;
	}
}
