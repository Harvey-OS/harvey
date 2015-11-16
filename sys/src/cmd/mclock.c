/*
 * mclock.c - graphical clock for Plan 9 using draw(2) API
 *
 * Graphical image is based on a clock program for Tektronix vector
 * graphics displays written in PDP-11 BASIC by Dave Robinson at the
 * University of Delaware in the 1970s.
 *
 * 071218 - initial release
 * 071223 - fix window label, fix redraw after hide, add tongue
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

int anghr, angmin, dia, offx, offy;
Image *dots, *back, *blk, *wht, *red, *org, *flesh;
Tm *mtime;

enum   {DBIG=600,
	XDarkOrange=0xff8c0000,
	Xwheat=0xf5deb300};

/* hair is head[0..41*2], face is head[27*2..56*2] */
int head[] = {286,386,263,410,243,417,230,415,234,426,227,443,210,450,190,448,
	172,435,168,418,175,400,190,398,201,400,188,390,180,375,178,363,
	172,383,157,390,143,388,130,370,125,350,130,330,140,318,154,318,
	165,325,176,341,182,320,195,305,200,317,212,322,224,319,218,334,
	217,350,221,370,232,382,250,389,264,387,271,380,275,372,276,381,
	279,388,286,386,300,360,297,337,294,327,284,320,300,301,297,297,
	282,286,267,284,257,287,254,280,249,273,236,274,225,290,195,305};
int mouth[] = {235,305,233,297,235,285,243,280,250,282,252,288,248,290,235,305};
int mouth1[] = {240,310,235,305,226,306};
int mouth2[] = {257,287,248,290};
int tongue[] = {235,285,243,280,246,281,247,286,245,289,241,291,237,294,233,294,
	235,285};
int tongue1[] = {241,291,241,286};
int shirt[] = {200,302,192,280,176,256,170,247,186,230,210,222,225,226,237,235,
	222,291,200,302};
int pants[] = {199,164,203,159,202,143,189,138,172,135,160,137,160,166,151,170,
	145,180,142,200,156,230,170,247,186,230,210,222,225,226,237,235,
	245,205,242,190,236,176,229,182,243,153,240,150,228,142,217,145,
	212,162,199,164};
int eyel[] = {294,327,296,335,293,345,285,345,280,337,281,325,284,320,294,327};
int eyer[] = {275,320,278,337,275,345,268,344,260,333,260,323,264,316,275,320};
int pupill[] = {284,320,294,327,293,329,291,333,289,333,286,331,284,325,284,320};
int pupilr[] = {265,316,275,320,275,325,273,330,271,332,269,333,267,331,265,327,
	265,316};
int nose[] = {285,308,288,302,294,301,298,301,302,303,305,305,308,308,309,310,
	310,312,310,316,308,320,305,323,302,324,297,324,294,322,288,317,
	286,312,285,308};
int nose1[] = {275,313,280,317,286,319};
int buttonl[] = {201,210,194,208,190,196,191,187,199,188,208,200,201,210};
int buttonr[] = {224,213,221,209,221,197,228,191,232,200,230,211,224,213};
int tail[] = {40,80,50,76,66,79,90,102,106,151,128,173,145,180};
int cuffl[] = {202,143,197,148,188,150,160,137};
int cuffr[] = {243,153,233,154,217,145};
int legl[] = {239,153,244,134,243,96,229,98,231,130,226,150,233,154,239,153};
int legr[] = {188,150,187,122,182,92,168,91,172,122,173,143,188,150};
int shoel[] = {230,109,223,107,223,98,228,90,231,76,252,70,278,73,288,82,
	284,97,271,99,251,100,244,106,230,109};
int shoel1[] = {223,98,229,98,243,96,251,100};
int shoel2[] = {271,99,248,89};
int shoer[] = {170,102,160,100,160,92,163,85,157,82,160,73,178,66,215,63,
	231,76,228,90,213,97,195,93,186,93,187,100,184,102,170,102};
int shoer1[] = {160,92,168,91,182,92,186,93};
int shoer2[] = {195,93,182,83};
int tick1[] = {302,432,310,446};
int tick2[] = {370,365,384,371};
int tick3[] = {395,270,410,270};
int tick4[] = {370,180,384,173};
int tick5[] = {302,113,310,100};
int tick7[] = {119,113,110,100};
int tick8[] = {40,173,52,180};
int tick9[] = {10,270,25,270};
int tick10[] = {40,371,52,365};
int tick11[] = {110,446,119,432};
int tick12[] = {210,455,210,470};
int armh[] = {-8,0,9,30,10,70,8,100,20,101,23,80,22,30,4,-5};
int armm[] = {-8,0,10,80,8,130,22,134,25,80,4,-5};
int handm[] = {8,140,5,129,8,130,22,134,30,137,27,143,33,163,30,168,
	21,166,18,170,12,168,10,170,5,167,4,195,-4,195,-6,170,
	0,154,8,140};
int handm1[] = {0,154,5,167};
int handm2[] = {14,167,12,158,10,152};
int handm3[] = {12,158,18,152,21,166};
int handm4[] = {20,156,29,151};
int handh[] = {20,130,15,135,6,129,4,155,-4,155,-6,127,-8,121,4,108,
	3,100,8,100,20,101,23,102,21,108,28,126,24,132,20,130};
int handh1[] = {20,130,16,118};

void
xlate(int* in, Point* out, int np)
{
	int i;

	for (i = 0; i < np; i++) {
		out[i].x = offx + (dia * (in[2*i]) + 210) / 420;
		out[i].y = offy + (dia * (480 - in[2*i+1]) + 210) / 420;
	}
}

void
myfill(int* p, int np, Image* color)
{
	Point* out;

	out = (Point *)malloc(sizeof(Point) * np);
	xlate(p, out, np);
	fillpoly(screen, out, np, ~0, color, ZP);
	free(out);
}

void
mypoly(int* p, int np, Image* color)
{
	Point* out;

	out = (Point *)malloc(sizeof(Point) * np);
	xlate(p, out, np);
	poly(screen, out, np, Enddisc, Enddisc, dia>DBIG?1:0, color, ZP);
	free(out);
}

void
arm(int* p, Point* out, int np, double angle)
{
	int i;
	double cosp, sinp;

	for (i = 0; i < np; i++) {
		cosp = cos(PI * angle / 180.0);
		sinp = sin(PI * angle / 180.0);
		out[i].x = p[2*i] * cosp + p[2*i+1] * sinp + 210.5;
		out[i].y = p[2*i+1] * cosp - p[2*i] * sinp + 270.5;
	}
}

void
polyarm(int *p, int np, Image *color, double angle)
{
	Point *tmp, *out;

	tmp = (Point *)malloc(sizeof(Point) * np);
	out = (Point *)malloc(sizeof(Point) * np);
	arm(p, tmp, np, angle);
	xlate((int*)tmp, out, np);
	poly(screen, out, np, Enddisc, Enddisc, dia>DBIG?1:0, color, ZP);
	free(out);
	free(tmp);
}

void
fillarm(int *p, int np, Image *color, double angle)
{
	Point *tmp, *out;

	tmp = (Point *)malloc(sizeof(Point) * np);
	out = (Point *)malloc(sizeof(Point) * np);
	arm(p, tmp, np, angle);
	xlate((int*)tmp, out, np);
	fillpoly(screen, out, np, ~0, color, ZP);
	free(out);
	free(tmp);
}

void
arms(void)
{
	/* arms */
	fillarm(armh, 8, blk, anghr);
	fillarm(armm, 6, blk, angmin);

	/* hour hand */
	fillarm(handh, 16, wht, anghr);
	polyarm(handh, 16, blk, anghr);
	polyarm(handh1, 2, blk, anghr);

	/* minute hand */
	fillarm(handm, 18, wht, angmin);
	polyarm(handm, 18, blk, angmin);
	polyarm(handm1, 2, blk, angmin);
	polyarm(handm2, 3, blk, angmin);
	polyarm(handm3, 3, blk, angmin);
	polyarm(handm4, 2, blk, angmin);
}

void
redraw(Image *screen)
{
	anghr = mtime->hour*30 + mtime->min/2;
	angmin = mtime->min*6;

	dia = Dx(screen->r) < Dy(screen->r) ? Dx(screen->r) : Dy(screen->r);
	offx = screen->r.min.x + (Dx(screen->r) - dia) / 2;
	offy = screen->r.min.y + (Dy(screen->r) - dia) / 2;

	draw(screen, screen->r, back, nil, ZP);

	/* first draw the filled areas */
	myfill(head, 42, blk);  /* hair */
	myfill(&head[27*2], 29, flesh);  /* face */
	myfill(mouth, 8, blk);
	myfill(tongue, 9, red);
	myfill(shirt, 10, blk);
	myfill(pants, 26, red);
	myfill(buttonl, 7, wht);
	myfill(buttonr, 7, wht);
	myfill(eyel, 8, wht);
	myfill(eyer, 8, wht);
	myfill(pupill, 8, blk);
	myfill(pupilr, 9, blk);
	myfill(nose, 18, blk);
	myfill(shoel, 13, org);
	myfill(shoer, 16, org);
	myfill(legl, 8, blk);
	myfill(legr, 7, blk);

	/* outline the color-filled areas */
	mypoly(&head[27*2], 29, blk);  /* face */
	mypoly(tongue, 9, blk);
	mypoly(pants, 26, blk);
	mypoly(buttonl, 7, blk);
	mypoly(buttonr, 7, blk);
	mypoly(eyel, 8, blk);
	mypoly(eyer, 8, blk);
	mypoly(shoel, 13, blk);
	mypoly(shoer, 16, blk);

	/* draw the details */
	mypoly(nose1, 3, blk);
	mypoly(mouth1, 3, blk);
	mypoly(mouth2, 2, blk);
	mypoly(tongue1, 2, blk);
	mypoly(tail, 7, blk);
	mypoly(cuffl, 4, blk);
	mypoly(cuffr, 3, blk);
	mypoly(shoel1, 4, blk);
	mypoly(shoel2, 2, blk);
	mypoly(shoer1, 4, blk);
	mypoly(shoer2, 2, blk);
	mypoly(tick1, 2, dots);
	mypoly(tick2, 2, dots);
	mypoly(tick3, 2, dots);
	mypoly(tick4, 2, dots);
	mypoly(tick5, 2, dots);
	mypoly(tick7, 2, dots);
	mypoly(tick8, 2, dots);
	mypoly(tick9, 2, dots);
	mypoly(tick10, 2, dots);
	mypoly(tick11, 2, dots);
	mypoly(tick12, 2, dots);

	arms();

	flushimage(display, 1);
	return;
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		fprint(2,"can't reattach to window");
	redraw(screen);
}

void
main(void)
{
	Event e;
	Mouse m;
	Menu menu;
	char *mstr[] = {"exit", 0};
	int key, timer, oldmin;

	initdraw(0,0,"mclock");
	back = allocimagemix(display, DPalebluegreen, DWhite);

	dots = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DBlue);
	blk = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DBlack);
	wht = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DWhite);
	red = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DRed);
	org = allocimage(display, Rect(0,0,1,1), CMAP8, 1, XDarkOrange);
	flesh = allocimage(display, Rect(0,0,1,1), CMAP8, 1, Xwheat);

	mtime = localtime(time(0));
	redraw(screen);

	einit(Emouse);
	timer = etimer(0, 30*1000);

	menu.item = mstr;
	menu.lasthit = 0;

	for(;;) {
		key = event(&e);
		if(key == Emouse) {
			m = e.mouse;
			if(m.buttons & 4) {
				if(emenuhit(3, &m, &menu) == 0)
					exits(0);
			}
		} else if(key == timer) {
			oldmin = mtime->min;
			mtime = localtime(time(0));
			if(mtime->min != oldmin) redraw(screen);
		}
	}
}
