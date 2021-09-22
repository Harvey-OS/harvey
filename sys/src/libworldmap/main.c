#include	<u.h>
#include	<libc.h>
#include	<thread.h>
#include	<ctype.h>
#include	<bio.h>
#include	<draw.h>
#include	<keyboard.h>
#include	<mouse.h>
#include	<worldmap.h>
#include	"mapimpl.h"

int N = 500;
int res = 1;
long debug;
int mask = 0x7f;

Rectangle	r, fr;
Image		*chart, *red;
Image		*c[Nmaptype];
int			outline;
int			wflag;
char		*back;
int			see;

int			minichange;

Font		*fnt;

int wid, ht;
int lat, lng;

void	work(int, int);
void	resize(int f);

Mousectl	*mousectl;
Keyboardctl	*keyboardctl;

uchar minimap[180*360];

void
threadmain(int argc, char *argv[])
{
	int fd;
	char minimapname[64];

	lat = 40;
	lng = 286;

	ARGBEGIN {
	default:
		fprint(2, "usage: %s [-d] [-g] [-l lat lon] [-s scale] -w -o\n", argv0);
		exits(0);
	case 'b':
		back = ARGF();
		break;
	case 'c':
		see++;
		break;
	case 'd':
		debug = strtol(ARGF(), nil, 0);
		break;
	case 's':
		N = atof(ARGF());
		break;
	case 'l':
		lat = atoi(ARGF());
		lng = atoi(ARGF());
		break;
	case 'o':
		outline++;
		break;
	case 'w':
		wflag++;
		break;
	case 'r':
		res = atoi(ARGF());
		break;
	case 'm':
		mask = strtol(ARGF(), nil, 0);
		break;
	} ARGEND

	if (initdraw(0, 0, "map") < 0)
		sysfatal("initdraw %r");

	mousectl = initmouse(nil, screen);
	if (mousectl == nil)
		sysfatal("initmouse: %r");

	keyboardctl = initkeyboard(nil);
	if (keyboardctl == nil)
		sysfatal("initkeyboard: %r");

	r = Rect(0, 0, N, N);
	c[Coastline]= display->black;
	c[Land]		= allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, 0x9eee9eff);
	c[Sea]		= allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, 0xb0b0ffff);
	c[Border]	= allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, 0x808080ff);
	c[Stateline]= allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, 0xb0b0b0ff);
	c[River]	= c[Sea];
	c[Riverlet]	= c[Sea];

	mapinit();

	red = allocimage(display, Rect(0, 0, 1, 1), RGB24, 1, DRed);
	chart = allocimage(display, r, RGB24, 0, DNofill);

	sprint(minimapname, "%s/world.map", mapbase);

	if (see) {
		fnt = openfont(display, "/lib/font/bit/lucidasans/unicode.6.font");
		if (fnt == nil)
			sysfatal("openfont: %r");
	}
	if (lat < -90 || lat >= 90)
		return;
	while (lng <    0) lng += 360;
	while (lng >= 360) lng -= 360;

	switch (res) {
		case 0:
			threadexits(nil);
		case 1:
			break;
		case 2:
		case 5:
		case 10:
		case 20:
		default:
			lat = lat - ((lat + 90) % res);
			lng = lng - (lng % res);
	}

	resize(0);

	if ((fd = open(minimapname, OREAD)) < 0)
		sysfatal("%s: %r", minimapname);
	if (read(fd, minimap, 360*180) != 360*180)
		sysfatal("read %s: %r", minimapname);
	close(fd);

	for (;;) {
		int i;
		struct Mouse m;
		Rune ru;
		Alt av[] = {
			//	c					v		op
			{mousectl->c,			&m,		CHANRCV},	// [0]
			{mousectl->resizec,		&i,		CHANRCV},	// [1]
			{keyboardctl->c,		&ru,	CHANRCV},	// [2]
			{nil,					nil,	CHANEND},	// [3]
		};
		switch (alt(av)) {
		case 0:
			/* mouse click */
			if (ptinrect(m.xy, rectaddpt(Rect(0,0,wid*N,ht*N),screen->r.min))) {
				static int but;
				int lo, la;
				uchar col;

				if ((but & 0x7) &&(m.buttons & 0x7) == 0) {
					lo = ((m.xy.x-screen->r.min.x)/N);
					la = ((m.xy.y-screen->r.min.y)/N);
					lo = lng + (lo - wid/2)*res;
					la = lat + (ht/2 - la)*res;

					while (lo < 0) lo += 360;
					while (lo >= 360) lo -= 360;
					if (la >= -90 && la < 90) {
						col = minimap[360*(89-la)+lo];
						if (debug&0x20)
							fprint(2, "Color adjust for %d %d, old color %d",
								la, lo, col);
						if (but & 0x1)
							col = 123;
						else if (but & 0x2) {
							if (col == 188 || col == 123)
								col = col==188?123:188;
							else
								fprint(2, "can't reverse unknown background\n");
						} else if (but & 0x4)
							col = 188;
						if (debug&0x20)
							fprint(2, "new color %d\n", col);
						minimap[360*(89-la)+lo] = col;
						minichange++;
						work(la, lo);
						lo = ((m.xy.x-screen->r.min.x)/N);
						la = ((m.xy.y-screen->r.min.y)/N);
						draw(screen, rectaddpt(screen->r, Pt(lo*N,la*N)), chart, nil,
							chart->r.min);
						flushimage(display, 1);
					}
				}
				but = m.buttons & 0x7;
			} else {
				static int but;

				if ((but & 0x7) &&(m.buttons & 0x7) == 0) {
					resize(0);
				}
				but = m.buttons & 0x7;
			}
			break;
		case 1:
			resize(1);
			break;
		case 2:
			if (minichange) {
				if ((fd = open(minimapname, OWRITE)) < 0)
					sysfatal("%s: %r", minimapname);
				if (write(fd, minimap, 360*180) != 360*180)
					sysfatal("write %s: %r", minimapname);
				close(fd);
			}
			threadexitsall(0);
			break;
		}
	}
}

void
work(int lat, int lon) {
	Maptile *m;
	Point pt;

	if (debug)
		fprint(2, "work(%d, %d)\n", lat, lon);
	while (lon <    0) lon += 360;
	while (lon >= 360) lon -= 360;

	if (lat < -90 || lat >= 90) {
		draw(chart, r, red, nil, ZP);
		return;
	}

	m = readtile(lat, lon, res);

	if (m == 0 || (debug && wflag))
		draw(chart, r, red, nil, ZP);

	if (m) {
		pt = Pt(lon, lat);
		drawtile(m, mask, &pt);
		freetile(m);
	}
}

void
resize(int f) {
	int lo, la;
	char str[64];

	if (f && getwindow(display, Refbackup) < 0)
		sysfatal("getwindow: %r");

	wid = Dx(screen->r)/N;
	ht = Dy(screen->r)/N;

	for (lo = 0; lo < wid; lo++) {
		for (la = 0; la < ht; la++) {
			draw(chart, chart->r, red, nil, ZP);
			work(lat + (ht/2 - la)*res, lng + (lo - wid/2)*res);
			draw(screen, rectaddpt(screen->r, Pt(lo*N,la*N)),
				chart, nil, chart->r.min);
			if (fnt && lo == 0) {
				sprint(str, "%3.3d", lat + (ht/2 - la)*res);
				string(screen, addpt(screen->r.min, Pt(0,la*N+fnt->height)),
					display->black, ZP, fnt, str);
			}
		}
		if (fnt) {
			sprint(str, "%3.3d", (int)lng + (lo - wid/2)*res);
			string(screen, addpt(screen->r.min, Pt(lo*N,0)),
				display->black, ZP, fnt, str);
		}
	}
	flushimage(display, 1);
}

void
drawpoly(Poly *pol, int type, void *arg) {
	Point *pt, *latlon;
	int np, i;

	latlon = arg;
	pt = malloc(sizeof(Point)*pol->np);
	np = 0;
	for (i = 0; i < pol->np; i++) {
		pt[np] = Pt((pol->p[i].x - latlon->x*1000000)/res*N/1000000,
				N - (pol->p[i].y - latlon->y*1000000)/res*N/1000000);
		pt[np] = addpt(chart->r.min, pt[np]);
		if (np == 0 || !eqpt(pt[np], pt[np-1]))
			np++;
	}
	if (type == Land || type == Sea) {
		fillpoly(chart, pt, np, ~0, c[type], ZP);
		if (outline)
			poly(chart, pt, np, Enddisc, (debug&0xf0)?Endarrow:Enddisc, 0,
				display->black, ZP);
	} else
		poly(chart, pt, np, Enddisc, Enddisc, 0, c[type], ZP);

	if (debug && wflag) {
		Rune ru;

		draw(screen, screen->r, chart, nil, chart->r.min);
		flushimage(display, 1);
		recv(keyboardctl->c, &ru);
		if (ru == 'q') threadexitsall(nil);
	}
	free(pt);
}
