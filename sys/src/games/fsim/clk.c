#include	"type.h"

typedef	struct	Clk	Clk;
struct	Clk
{
	Rectangle	I;
	char		s1[10];
	char		s2[10];
	Point		tim;
	Point		stp;
	long		t;
};
static	Clk	clk;

void
clkinit(void)
{

	clk.I.min.x = plane.origx + 0*plane.side;
	clk.I.min.y = plane.origy + 2*plane.side;
	clk.I.max.x = clk.I.min.x + plane.side;
	clk.I.max.y = clk.I.min.y + plane.side;
	rectf(D, clk.I, F_CLR);
	circle(D, add(clk.I.min, Pt(plane.side24, plane.side24)),
		plane.side24, LEVEL, F_STORE);
	clk.tim.x = clk.I.min.x + plane.side24 - 3*W;
	clk.tim.y = clk.I.min.y + plane.side24 - (3*H)/2;
	clk.stp.x = clk.I.min.x + plane.side24 - 3*W;
	clk.stp.y = clk.I.min.y + plane.side24 + H/2;
	strcpy(clk.s1, "");
	strcpy(clk.s2, "");
	clk.t = 0;
}

void
clkupdat(void)
{
	char s[10];
	int t;

	if(plane.nbut && !plane.obut) {
		t = hit(clk.stp.x, clk.stp.y, 6);
		if(t >= 0)
			clk.t = plane.time;
	}
	if(plane.time % 10)
		return;
	strcpy(s, "XXX:XX");
	t = plane.time / 600L;
	dconv(t, s, 3);
	t = (plane.time % 600L) / 10L;
	dconv(t, s+4, 2);
	string(D, clk.tim, font, s, F_XOR);
	string(D, clk.tim, font, clk.s1, F_XOR);
	strcpy(clk.s1, s);

	strcpy(s, "XXX:XX");
	t = (plane.time - clk.t) / 600L;
	dconv(t, s, 3);
	t = ((plane.time - clk.t) % 600L) / 10L;
	dconv(t, s+4, 2);
	string(D, clk.stp, font, s, F_XOR);
	string(D, clk.stp, font, clk.s2, F_XOR);
	strcpy(clk.s2, s);
}
