#include	"type.h"

typedef	struct	Adf	Adf;
struct	Adf
{
	Rectangle	I;
	char		s1[10];
	char		s2[10];
	Point		freq;
	Point		dme;
	int		na;
	long		ah1[50];
	Image*		i;
	Image*		t;
};
static	Adf	adf;
static	long	adfh1[] =
{
	-25+500, -400+500,
	-25+500, 400+500,
	-75+500, 400+500,
	0+500, 475+500,
	75+500, 400+500,
	25+500, 400+500,
	25+500, -400+500,
	-25+500, -400+500,
	NONE
};

void
adfinit(void)
{
	int dx, dy, i;
	Fract is, ic;
	long d1, d2;

	adf.I.min.x = plane.origx + 4*plane.side;
	adf.I.min.y = plane.origy + 0*plane.side;
	adf.I.max.x = adf.I.min.x + plane.side;
	adf.I.max.y = adf.I.min.y + plane.side;

	d_bfree(adf.i);
	d_bfree(adf.t);
	adf.i = d_balloc(adf.I, 0);
	adf.t = d_balloc(adf.I, 0);

	dx = adf.I.min.x + plane.side24;
	dy = adf.I.min.y + plane.side24;
	d2 = plane.side24;
	for(i=0; i<360; i+=10) {
		is = isin(DEG(i));
		ic = icos(DEG(i));
		d1 = muldiv(plane.side24, 90, 100);
		if(i%30 == 0)
			d1 = muldiv(plane.side24, 80, 100);
		d_segment(adf.i,
			Pt(dx + (int)fmul(d1, is),
				dy + (int)fmul(d1, ic)),
			Pt(dx + (int)fmul(d2, is),
				dy + (int)fmul(d2, ic)),
			LEVEL);
	}
	d_circle(adf.i, addpt(adf.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL);
	adf.freq.x = adf.I.min.x + plane.side24 - (3*W)/2;
	adf.freq.y = adf.I.min.y + plane.side - (3*H)/2;
	adf.dme.x = adf.I.min.x + plane.side24 - (5*W)/2;
	adf.dme.y = adf.I.min.y + plane.side - (5*H)/2;
	adf.na = NONE;
	strcpy(adf.s1, "");
	strcpy(adf.s2, "");
	normal(adf.ah1, adfh1);
}

void
adfupdat(void)
{
	long d, z, t;

	d_copy(adf.t, adf.I, adf.i);
	if(plane.nbut && !plane.obut) {
		t = hit(adf.freq.x, adf.freq.y, 3);
		if(t >= 0)
			plane.adf = mod3(plane.adf, t);
	}

	adf.na = NONE;
	if(plane.ad) {
		adf.na = deg(bearto(plane.ad) - plane.ah);
		dodraw(adf.t, adf.na, adf.ah1, &adf.I);
	}

	strcpy(adf.s1, "XXX");
	dconv(plane.adf, adf.s1, 3);
	d_string(adf.t, adf.freq, font, adf.s1);

	strcpy(adf.s2, "XXX.X");
	if(plane.ad)
	if(plane.ad->flag & DME) {
		z = plane.z - plane.ad->z;
		if(z < 0)
			z = -z;
		d = fdist(plane.ad);
		t = ihyp(d, z) / (MILE(1)/10);
		if(t > 9999)
			t = 9999;
		dconv(t/10, adf.s2, 3);
		dconv(t%10, adf.s2+4, 1);
	}
	d_string(adf.t, adf.dme, font, adf.s2);
	d_copy(D, adf.I, adf.t);
}
