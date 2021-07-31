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

	adf.I.min.x = plane.origx + 3*plane.side;
	adf.I.min.y = plane.origy + 2*plane.side;
	adf.I.max.x = adf.I.min.x + plane.side;
	adf.I.max.y = adf.I.min.y + plane.side;
	rectf(D, adf.I, F_CLR);
	dx = adf.I.min.x + plane.side24;
	dy = adf.I.min.y + plane.side24;
	d2 = plane.side24;
	for(i=0; i<360; i+=10) {
		is = isin(DEG(i));
		ic = icos(DEG(i));
		d1 = muldiv(plane.side24, 90, 100);
		if(i%30 == 0)
			d1 = muldiv(plane.side24, 80, 100);
		segment(D,
			Pt(dx + (int)fmul(d1, is),
				dy + (int)fmul(d1, ic)),
			Pt(dx + (int)fmul(d2, is),
				dy + (int)fmul(d2, ic)),
			LEVEL, F_STORE);
	}
	circle(D, add(adf.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	adf.freq.x = adf.I.min.x + plane.side24 - (3*W)/2;
	adf.freq.y = adf.I.min.y + plane.side - (3*H)/2;
	adf.dme.x = adf.I.min.x + plane.side24 - (5*W)/2;
	adf.dme.y = adf.I.min.y + plane.side - (5*H)/2;
	adf.na = NONE;
	strcpy(adf.s1, "");
	strcpy(adf.s2, "");
	normal(adfh1);
}

void
adfupdat(void)
{
	char s[20];
	long d, z, t;

	if(plane.nbut && !plane.obut) {
		t = hit(adf.freq.x, adf.freq.y, 3);
		if(t >= 0)
			plane.adf = mod3(plane.adf, t);
	}

	t = NONE;
	if(plane.ad)
		t = deg(bearto(plane.ad->x, plane.ad->y) - plane.ah);
	if(adf.na != t) {
		if(t != NONE)
			draw(t, adfh1, &adf.I);
		if(adf.na != NONE)
			draw(adf.na, adfh1, &adf.I);
	}
	adf.na = t;

	strcpy(s, "XXX");
	dconv(plane.adf, s, 3);
	if(strcmp(s, adf.s1)) {
		string(D, adf.freq, font, s, F_XOR);
		string(D, adf.freq, font, adf.s1, F_XOR);
		strcpy(adf.s1, s);
	}

	strcpy(s, "XXX.X");
	if(plane.ad)
	if(plane.ad->flag & DME) {
		z = plane.z - plane.ad->z;
		if(z < 0)
			z = -z;
		d = fdist(plane.ad->x, plane.ad->y);
		t = ihyp(d, z) / (MILE/10);
		if(t > 9999)
			t = 9999;
		dconv(t/10, s, 3);
		dconv(t%10, s+4, 1);
	}
	if(strcmp(s, adf.s2)) {
		string(D, adf.dme, font, s, F_XOR);
		string(D, adf.dme, font, adf.s2, F_XOR);
		strcpy(adf.s2, s);
	}
}
