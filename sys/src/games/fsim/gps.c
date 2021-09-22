#include	"type.h"

enum
{
	Cleft	= 6,
	Cright	= 16,

	Papt	= 0,
	Pvor,
	Pndb,
	Pint,
	Pusr,
	Pact,
	Pnav,
	Pfpl,
	Pcal,
	Pset,
	Poth,

	Bmsg	= 0,
	Bobs,
	Balt,
	Bnrst,
	Bd,
	Bclr,
	Bent,

	Bcrsr,
	Bskl,
	Bskr,
	Blkl,
	Blkr,
};

typedef	struct	Gps	Gps;
struct	Gps
{
	/* main unit */
	Rectangle	I;
	Image*		i;
	Image*		t;
	Rectangle	button[7];
	Rectangle	label[11];
	Rectangle	chars[4][Cleft+Cright+1];
	Rectangle	window;

	/* annunciator */
	Rectangle	J;
	Image*		j;
	Image*		u;
	Point		knob;
	int		smallrad;
	int		largerad;
	Rectangle	cursor;
	Rectangle	smallknobl;
	Rectangle	smallknobr;
	Rectangle	largeknobl;
	Rectangle	largeknobr;

	/* page data */
	char		left[4][30];
	char		right[4][30];
	Nav*		nearvor;
	uchar		pagetype;
	uchar		pagenumber;
	uchar		obsmode;
	uchar		crsrmode;
	uchar		firstpage[11];
	uchar		lastpage[11];
	Nav*		fpl[10];
};
static	Gps	gps;
static	void	gpspage(void);
static	void	gpsbut(int);

static void
d_outline(Image *i, Rectangle r)
{
	d_segment(i, Pt(r.min.x, r.min.y), Pt(r.max.x-1, r.min.y), LEVEL);
	d_segment(i, Pt(r.max.x-1, r.min.y), Pt(r.max.x-1, r.max.y-1), LEVEL);
	d_segment(i, Pt(r.max.x-1, r.max.y-1), Pt(r.min.x, r.max.y-1), LEVEL);
	d_segment(i, Pt(r.min.x, r.max.y-1), Pt(r.min.x, r.min.y), LEVEL);
}

static void
even(Rectangle *r, int n, int len)
{
	int i;

	for(i=0; i<n; i++) {
		r->min.x = len - muldiv((n-i)*1000, len, 1000*n+100);
		r->max.x = muldiv((i+1)*1000, len, 1000*n+100);
		r++;
	}
}

static void
d_label(Image *i, Rectangle *r, char *s)
{
	int w;

	w = stringwidth(font, s);
	d_string(i, Pt((r->min.x+r->max.x-w)/2, (r->min.y+r->max.y-H)/2), font, s);
}

static int
inside(Rectangle *r)
{
	if(plane.buty < r->min.y)
		return 0;
	if(plane.buty > r->max.y)
		return 0;
	if(plane.butx < r->min.x)
		return 0;
	if(plane.butx > r->max.x)
		return 0;
	return 1;
}

static	char*
lab[] =
{
	"MSG",
	"VOR",
	"NDB",
	"INT",
	"USR",
	"ACT",
	"NAV",
	"FPL",
	"CAL",
	"SET",
	"OTH",
};

static	char*
but[] =
{
	"MSG",
	"OBS",
	"ALT",
	"NRS",
	"Ð",
	"CLR",
	"ENT",
};

void
gpsinit(void)
{
	int i, j;

	/*
	 * main unit
	 */
	gps.I.min.x = plane.origx + 3*plane.side;
	gps.I.min.y = plane.origy + 2*plane.side;
	gps.I.max.x = gps.I.min.x + 2*plane.side;
	gps.I.max.y = gps.I.min.y + plane.side;

	d_bfree(gps.i);
	d_bfree(gps.t);
	gps.i = d_balloc(gps.I, 0);
	gps.t = d_balloc(gps.I, 0);
	d_outline(gps.i, gps.I);

	gps.window.min.x = gps.I.min.x + muldiv(plane.side, 100, 1000);
	gps.window.max.x = gps.I.min.x + muldiv(plane.side, 1900, 1000);
	gps.window.min.y = gps.I.min.y + muldiv(plane.side, 100, 1000);
	gps.window.max.y = gps.I.min.y + muldiv(plane.side, 600, 1000);
	d_outline(gps.i, gps.window);

	even(gps.label, nelem(gps.label), muldiv(plane.side, 1800, 1000));
	for(i=0; i<nelem(gps.label); i++) {
		gps.label[i].min.x += gps.I.min.x + muldiv(plane.side, 100, 1000);
		gps.label[i].max.x += gps.I.min.x + muldiv(plane.side, 100, 1000);
		gps.label[i].min.y = gps.I.min.y + muldiv(plane.side, 625, 1000);
		gps.label[i].max.y = gps.I.min.y + muldiv(plane.side, 750, 1000);
		d_outline(gps.i, gps.label[i]);
		j = lab[i][1];
		lab[i][1] = 0;
		d_label(gps.i, gps.label+i, lab[i]);
		lab[i][1] = j;
	}
	even(gps.button, nelem(gps.button), muldiv(plane.side, 1800, 1000));
	for(i=0; i<nelem(gps.button); i++) {
		gps.button[i].min.x += gps.I.min.x + muldiv(plane.side, 100, 1000);
		gps.button[i].max.x += gps.I.min.x + muldiv(plane.side, 100, 1000);
		gps.button[i].min.y = gps.I.min.y + muldiv(plane.side, 800, 1000);
		gps.button[i].max.y = gps.I.min.y + muldiv(plane.side, 925, 1000);
		d_outline(gps.i, gps.button[i]);
		d_label(gps.i, gps.button+i, but[i]);
	}
	for(i=0; i<nelem(gps.chars); i++) {
		even(gps.chars[i], nelem(gps.chars[i]), muldiv(plane.side, 1800, 1000));
		for(j=0; j<nelem(gps.chars[i]); j++) {
			gps.chars[i][j].min.x += gps.window.min.x;
			gps.chars[i][j].max.x += gps.window.min.x;
			gps.chars[i][j].min.y = gps.window.min.y +
				muldiv(i, gps.window.max.y-gps.window.min.y-4, 4) + H/2;
			gps.chars[i][j].max.y = gps.I.min.y +
				muldiv(i+1, gps.window.max.y-gps.window.min.y-4, 4) + H/2;
		}
		
	}
	i = (gps.chars[0][6].min.x + gps.chars[0][6].max.x) / 2;
	d_segment(gps.i, Pt(i, gps.window.min.y), Pt(i, gps.window.max.y), LEVEL);

	/*
	 * annunciator
	 */
	gps.J.min.x = plane.origx + 4*plane.side;
	gps.J.min.y = plane.origy + 1*plane.side + plane.side24;
	gps.J.max.x = gps.J.min.x + plane.side;
	gps.J.max.y = gps.J.min.y + plane.side24;

	d_bfree(gps.j);
	d_bfree(gps.u);
	gps.j = d_balloc(gps.J, 0);
	gps.u = d_balloc(gps.J, 0);

	gps.knob.x = gps.J.max.x - plane.side14;
	gps.knob.y = gps.J.max.y - plane.side14;
	gps.smallrad = muldiv(plane.side, 120, 1000);
	gps.largerad = muldiv(plane.side, 230, 1000);
	d_circle(gps.j, gps.knob, gps.smallrad, LEVEL);
	d_circle(gps.j, gps.knob, gps.largerad, LEVEL);

	gps.smallknobl.min.x = gps.knob.x -gps.smallrad;
	gps.smallknobl.min.y = gps.knob.y -gps.smallrad;
	gps.smallknobl.max.x = gps.knob.x;
	gps.smallknobl.max.y = gps.knob.y +gps.smallrad;

	gps.largeknobl.min.x = gps.knob.x -gps.largerad;
	gps.largeknobl.min.y = gps.knob.y -gps.largerad;
	gps.largeknobl.max.x = gps.knob.x;
	gps.largeknobl.max.y = gps.knob.y +gps.largerad;

	gps.smallknobr.min.x = gps.knob.x;
	gps.smallknobr.min.y = gps.knob.y -gps.smallrad;
	gps.smallknobr.max.x = gps.knob.x +gps.smallrad;
	gps.smallknobr.max.y = gps.knob.y +gps.smallrad;

	gps.largeknobr.min.x = gps.knob.x;
	gps.largeknobr.min.y = gps.knob.y -gps.largerad;
	gps.largeknobr.max.x = gps.knob.x +gps.largerad;
	gps.largeknobr.max.y = gps.knob.y +gps.largerad;

	d_segment(gps.j,
		Pt(gps.knob.x, gps.knob.y-gps.largerad),
		Pt(gps.knob.x, gps.knob.y+gps.largerad), LEVEL);

	i = muldiv(plane.side, 150, 1000);
	gps.cursor.min.x = gps.J.min.x + plane.side14 - i;
	gps.cursor.max.x = gps.J.min.x + plane.side14 + i;
	i = muldiv(plane.side, 100, 1000);
	gps.cursor.min.y = gps.J.min.y + plane.side14 - i;
	gps.cursor.max.y = gps.J.min.y + plane.side14 + i;
	d_outline(gps.j, gps.cursor);
	d_label(gps.j, &gps.cursor, "CRSR");

	d_outline(gps.j, gps.J);

	gps.firstpage[Papt] = 1;
	gps.firstpage[Pvor] = 1;
	gps.firstpage[Pndb] = 1;
	gps.firstpage[Pint] = 1;
	gps.firstpage[Pusr] = 1;
	gps.firstpage[Pact] = 1;
	gps.firstpage[Pnav] = 1;
	gps.firstpage[Pfpl] = 0;
	gps.firstpage[Pcal] = 1;
	gps.firstpage[Pset] = 1;
	gps.firstpage[Poth] = 1;

	gps.lastpage[Papt] = 8;
	gps.lastpage[Pvor] = 2;
	gps.lastpage[Pndb] = 2;
	gps.lastpage[Pint] = 2;
	gps.lastpage[Pusr] = 2;
	gps.lastpage[Pact] = 1;
	gps.lastpage[Pnav] = 4;
	gps.lastpage[Pfpl] = 0;
	gps.lastpage[Pcal] = 8;
	gps.lastpage[Pset] = 11;
	gps.lastpage[Poth] = 12;

	gps.pagetype = Pnav;
	gps.pagenumber = gps.firstpage[gps.pagetype];
	gps.obsmode = 0;
	gps.crsrmode = 0;

	gps.fpl[0] = lookup("MMU", APT);
	gps.fpl[1] = lookup("SBJ", VOR);
	gps.fpl[2] = lookup("PTW", VOR);
	gps.fpl[3] = lookup("BAL", VOR);
	gps.fpl[4] = lookup("DCA", APT);
}

void
gpsupdat(void)
{
	int i, j;
	Rune r;
	char ts[10];
	char *s;

	d_copy(gps.t, gps.I, gps.i);
	d_copy(gps.u, gps.J, gps.j);
	if(plane.nbut && inside(&gps.I)) {
		for(i=0; i<nelem(gps.button); i++) {
			if(inside(gps.button+i)) {
				d_set(gps.t, gps.button[i]);
				sprint(gps.right[0], "hit %d", i);
				if(!plane.obut)
					gpsbut(i);
			}
		}
	}
	if(plane.nbut && inside(&gps.J)) {
		if(inside(&gps.largeknobl)) {
			if(inside(&gps.smallknobl)) {
				if(!plane.obut)
					gpsbut(Bskl);
			} else {
				if(!plane.obut)
					gpsbut(Blkl);
			}
		} else
		if(inside(&gps.largeknobr)) {
			if(inside(&gps.smallknobr)) {
				if(!plane.obut)
					gpsbut(Bskr);
			} else {
				if(!plane.obut)
					gpsbut(Blkr);
			}
		} else
		if(inside(&gps.cursor)) {
			if(!plane.obut)
				gpsbut(Bcrsr);
		}
	}
	gpspage();

	d_set(gps.t,
		Rect(gps.label[gps.pagetype].min.x, gps.window.max.y-5,
			gps.label[gps.pagetype].max.x, gps.window.max.y));
	for(i=0; i<nelem(gps.chars); i++) {
		s = gps.left[i];
		for(j=0; j<Cleft; j++) {
			s += chartorune(&r, s);
			if(r == 0)
				break;
			sprint(ts, "%C", r);
			d_label(gps.t, gps.chars[i]+j, ts);
		}
		s = gps.right[i];
		for(j=0; j<Cright; j++) {
			s += chartorune(&r, s);
			if(r == 0)
				break;
			sprint(ts, "%C", r);
			d_label(gps.t, gps.chars[i]+Cleft+1+j, ts);
		}
	}

	d_copy(D, gps.I, gps.t);
	d_copy(D, gps.J, gps.u);
}

#define	C(a,b)	(((a)<<4)|(b))
static void
gpspage(void)
{
	int i, d, a;

	for(i=0; i<4; i++) {
		gps.left[i][0] = 0;
		gps.right[i][0] = 0;
	}
	sprint(gps.left[2], "%s%s",
		gps.crsrmode? "ent": "   ",
		gps.obsmode? "Obs": "Leg");
	sprint(gps.left[3], "%s %d", lab[gps.pagetype], gps.pagenumber);

	switch(C(gps.pagetype, gps.pagenumber)) {
	case C(Pnav, 2):
		sprint(gps.right[0], ">Present Posn");
		if(gps.nearvor == nil)
			break;
		sprint(gps.right[2], " Ref: %s", gps.nearvor->name);
		d = muldiv(fdist(gps.nearvor), 10, MILE(1));
		a = bearto(gps.nearvor)/DEG(1) + 180;
		if(a >= 360)
			a -= 360;
		if(d < 1000)
			sprint(gps.right[3], " %.3d°FR %2d.%dnm", a, d/10, d%10);
		else
			sprint(gps.right[3], " %.3d°FR %4dnm", a, d/10);
		break;
	case C(Pfpl, 0):
		for(i=0; gps.fpl[i]; i++) {
			sprint(gps.right[i], " %2d %s", i+1, gps.fpl[i]->name);
			if(i >= 3)
				break;
		}
		break;
	}
}

static void
gpsnewpage(void)
{
	Nav *nv;
	long bd, d;

	switch(C(gps.pagetype, gps.pagenumber)) {
	case C(Pnav, 2):
		gps.nearvor = nil;
		bd = MILE(1000);
		for(nv=nav; nv->name; nv++) {
			if((nv->flag & VOR) == 0)
				continue;
			d = fdist(nv);
			if(d < bd) {
				gps.nearvor = nv;
				bd = d;
			}
		}
		break;
	case C(Pfpl, 0):
		break;
	}
}

static void
gpsbut(int b)
{
	switch(b) {
	case Bmsg:
		break;
	case Bobs:
		gps.obsmode = !gps.obsmode;
		break;
	case Balt:
		break;
	case Bnrst:
		break;
	case Bd:
		break;
	case Bclr:
		break;
	case Bent:
		break;
	case Bcrsr:
		gps.crsrmode = !gps.crsrmode;
		break;
	case Bskl:
		if(gps.crsrmode) {
		} else {
			if(gps.pagenumber == gps.firstpage[gps.pagetype])
				gps.pagenumber = gps.lastpage[gps.pagetype];
			else
				gps.pagenumber -= 1;
		}
		gpsnewpage();
		break;
	case Bskr:
		if(gps.crsrmode) {
		} else {
			if(gps.pagenumber == gps.lastpage[gps.pagetype])
				gps.pagenumber = gps.firstpage[gps.pagetype];
			else
				gps.pagenumber += 1;
		}
		gpsnewpage();
		break;
	case Blkl:
		if(gps.crsrmode) {
		} else {
			if(gps.pagetype == Papt)
				gps.pagetype = Poth;
			else
				gps.pagetype -= 1;
			gps.pagenumber = gps.firstpage[gps.pagetype];
		}
		gpsnewpage();
		break;
	case Blkr:
		if(gps.crsrmode) {
		} else {
			if(gps.pagetype == Poth)
				gps.pagetype = Papt;
			else
				gps.pagetype += 1;
			gps.pagenumber = gps.firstpage[gps.pagetype];
		}
		gpsnewpage();
		break;
	}
}
